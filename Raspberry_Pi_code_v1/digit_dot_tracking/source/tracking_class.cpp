// File Name: tracking.cpp
// Author: Shaoxiong Wang
// Create Time: 2018/12/20 10:11
// Modified by Cole Ten (5/5/2023)

#include "tracking_class.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

using namespace std;

inline double squared_dist(Point2D a, Point2D b)
{
    Point2D coord_diff = a - b;
    return coord_diff.x * coord_diff.x + coord_diff.y * coord_diff.y;
}


inline double squared_dist_from_origin(Point2D a)
{
    return a.x * a.x + a.y * a.y;
}

int Point2DMatcher::precessor(int i, int j)
{
    return (
        curr_detected_point_pairwise_angles[i][j] <= theta
        && curr_detected_point_pairwise_angles[i][j] >= -theta
        && curr_detected_point_pairwise_squared_dists[i][j] <= dmax
        && curr_detected_point_pairwise_squared_dists[i][j] >= dmin
    );
}

Point2DMatcher::Point2DMatcher(
    uint8_t num_grid_rows,
    uint8_t num_grid_cols,
    uint16_t camera_fps,
    double point0_x_coord,
    double point0_y_coord,
    double x_grid_spacing,
    double y_grid_spacing
):
    camera_fps(camera_fps),
    is_first_time_step(true),
    num_grid_rows(num_grid_rows),
    num_grid_cols(num_grid_cols),
    actual_num_points(num_grid_rows * num_grid_cols),
    x_grid_spacing(x_grid_spacing),
    y_grid_spacing(y_grid_spacing)
{
    // Set initial detected points to assumed locations based on knowledge of point grid
    int r, c;
    for (r = 0; r < num_grid_rows; r++) {
        for (c = 0; c < num_grid_cols; c++) {
            initial_detected_points[r][c].x = point0_x_coord + c * x_grid_spacing;
            initial_detected_points[r][c].y = point0_y_coord + r * y_grid_spacing;
        }
    }

    dmin = (x_grid_spacing * 0.5) * (x_grid_spacing * 0.5);
    dmax = (x_grid_spacing * 1.8) * (x_grid_spacing * 1.8);
    theta = 70;
    moving_max = x_grid_spacing * 2;
    flow_difference_threshold = x_grid_spacing * 0.8;
    cost_threshold = 15000 * (x_grid_spacing / 21) * (x_grid_spacing / 21);
}

void Point2DMatcher::update_detected_points(
    std::vector<std::vector<double>> detected_points
)
{
    int i, j;

    // Update array of current points to equal current detected points
    this->curr_num_detected_points = static_cast<uint16_t>(detected_points.size());
    for (i = 0; i < curr_num_detected_points; i++) {
        this->curr_detected_points[i].x = detected_points[i][0];
        this->curr_detected_points[i].y = detected_points[i][1];
        this->curr_detected_points[i].id = i;
    }

    // Reset arrays/values used in depth-first search
    fill(done, done + sizeof(done), false);
    memset(occupied, -1, sizeof(occupied));
    minf = -1;

    // Sort by x-axis, if same by y-axis
    sort(curr_detected_points, curr_detected_points + curr_num_detected_points);

    // Calculate pairwise squared distance and angles (relative to x-axis)
    // between every (unordered) pair of current detected points.
    for (i = 0; i < curr_num_detected_points; i++) {
        for (j = 0; j < i; j++) {
            this->curr_detected_point_pairwise_squared_dists[i][j] = squared_dist(
                curr_detected_points[i], curr_detected_points[j]
            );
            this->curr_detected_point_pairwise_angles[i][j] =
                asin(
                    fabs(curr_detected_points[i].y - curr_detected_points[j].y)
                    / sqrt(curr_detected_point_pairwise_squared_dists[i][j])
                )
                * 180.0 / M_PI;
        }
    }
}

void Point2DMatcher::match_points()
{
    this->most_recent_update_clock_tick = clock();

    uint16_t num_missing_markers = static_cast<uint16_t>(
        max(actual_num_points - curr_num_detected_points, 0)
    );
    uint16_t num_extra_markers = static_cast<uint16_t>(
        max(curr_num_detected_points - actual_num_points, 0)
    );

    dfs(0, 0, num_missing_markers, num_extra_markers);

    for (int t = 1; t <= 3; t++) {
        if (minf == -1) {
            // std::cout<<"TRY AGAIN!!"<<std::endl;
            fill(done, done + sizeof(done), false);
            memset(occupied, -1, sizeof(occupied));
            dfs(0, 0, num_missing_markers + 1, num_extra_markers + 1);
        }
    }
    int i;
    if (is_first_time_step) {
        this->is_first_time_step = false;
        for (i = 0; i < curr_num_detected_points; i++) {
            initial_detected_points[MinRow[i]][MinCol[i]].x = curr_detected_points[i].x;
            initial_detected_points[MinRow[i]][MinCol[i]].y = curr_detected_points[i].y;
        }
    }
    // std::cout<<"MINF "<<minf<<"\t\t";
}

std::tuple<vvd, vvd, vvd, vvd, vvd> Point2DMatcher::calc_marker_displacements()
{
    vvd Ox(num_grid_rows), Oy(num_grid_rows), Cx(num_grid_rows), Cy(num_grid_rows),
        Occupied(num_grid_rows);

    int i, j;
    for (i = 0; i < num_grid_rows; i++) {
        Ox[i] = vd(num_grid_cols);
        Oy[i] = vd(num_grid_cols);
        Cx[i] = vd(num_grid_cols);
        Cy[i] = vd(num_grid_cols);
        Occupied[i] = vd(num_grid_cols);
        for (j = 0; j < num_grid_cols; j++) {
            Ox[i][j] = initial_detected_points[i][j].x;
            Oy[i][j] = initial_detected_points[i][j].y;
            Cx[i][j] = MinD[i][j].x;
            Cy[i][j] = MinD[i][j].y;
            Occupied[i][j] = MinOccupied[i][j];
            // Point2D a(matcher.O[i][j].x, matcher.O[i][j].y), b(matcher.MinD[i][j].x +
            // 2 * (matcher.MinD[i][j].x - matcher.O[i][j].x), matcher.MinD[i][j].y + 2
            // * (matcher.MinD[i][j].y - matcher.O[i][j].y));
        }
    }

    return std::make_tuple(Ox, Oy, Cx, Cy, Occupied);
}

double Point2DMatcher::calc_cost(int i)
{
    double c = 0, cost = 0;
    int left, up, down;
    Point2D flow1, flow2;

    cost = cost
           + K1
                 * squared_dist_from_origin(
                     curr_detected_points[i] - initial_detected_points[Row[i]][Col[i]]
                 );
    flow1 = curr_detected_points[i] - initial_detected_points[Row[i]][Col[i]];

    if (Col[i] > 0) {
        left = occupied[Row[i]][Col[i] - 1];
        if (left > -1) {
            flow2 = curr_detected_points[left]
                    - initial_detected_points[Row[i]][Col[i] - 1];
            c = squared_dist_from_origin(flow2 - flow1);
            if (sqrt(c) >= flow_difference_threshold)
                c = 1e8;
            cost += K2 * c;
        }
    }
    if (Row[i] > 0) {
        up = occupied[Row[i] - 1][Col[i]];
        if (up > -1) {
            flow2 = curr_detected_points[up]
                    - initial_detected_points[Row[i] - 1][Col[i]];
            c = squared_dist_from_origin(flow2 - flow1);
            if (sqrt(c) >= flow_difference_threshold)
                c = 1e8;
            cost += K2 * c;
        }
    }
    if (Row[i] < num_grid_rows - 1) {
        down = occupied[Row[i] + 1][Col[i]];
        if (down > -1) {
            flow2 = curr_detected_points[down]
                    - initial_detected_points[Row[i] + 1][Col[i]];
            c = squared_dist_from_origin(flow2 - flow1);
            if (sqrt(c) >= flow_difference_threshold)
                c = 1e8;
            cost += K2 * c;
        }
    }
    return cost;
}

double Point2DMatcher::infer()
{
    double cost = 0;
    int boarder_nb = 0;
    int i, j, k, x, y, d = 1, cnt, nx, ny, nnx, nny;

    int dir[4][2] = {{0, -1}, {-1, 0}, {0, 1}, {1, 0}};
    Point2D flow1, flow2;

    Point2D moving;

    for (i = 0; i < num_grid_rows; i++) {
        for (j = 0; j < num_grid_cols; j++) {
            if (occupied[i][j] <= -1) {
                moving.x = 0;
                moving.y = 0;
                cnt = 0;
                for (k = 0; k < 4; k++) {
                    nx = i + dir[k][0];
                    ny = j + dir[k][1];
                    nnx = nx + dir[k][0];
                    nny = ny + dir[k][1];
                    if (nnx < 0 || nnx >= num_grid_rows || nny < 0
                        || nny >= num_grid_cols)
                        continue;
                    if (occupied[nx][ny] <= -1 || occupied[nnx][nny] <= -1)
                        continue;
                    moving = moving
                             + (curr_detected_points[occupied[nx][ny]]
                                - initial_detected_points[nx][ny]
                                + (curr_detected_points[occupied[nx][ny]]
                                   - initial_detected_points[nx][ny]
                                   - curr_detected_points[occupied[nnx][nny]]
                                   + initial_detected_points[nnx][nny]));
                    cnt += 1;
                }
                if (cnt == 0) {
                    for (x = i - d; x <= i + d; x++) {
                        for (y = j - d; y <= j + d; y++) {
                            if (x < 0 || x >= num_grid_rows || y < 0
                                || y >= num_grid_cols)
                                continue;
                            if (occupied[x][y] <= -1)
                                continue;
                            moving = moving
                                     + (curr_detected_points[occupied[x][y]]
                                        - initial_detected_points[x][y]);
                            cnt += 1;
                        }
                    }
                }
                if (cnt == 0) {
                    for (x = i - d - 1; x <= i + d + 1; x++) {
                        for (y = j - d - 1; y <= j + d + 1; y++) {
                            if (x < 0 || x >= num_grid_rows || y < 0
                                || y >= num_grid_cols)
                                continue;
                            if (occupied[x][y] <= -1)
                                continue;
                            moving = moving
                                     + (curr_detected_points[occupied[x][y]]
                                        - initial_detected_points[x][y]);
                            cnt += 1;
                        }
                    }
                }
                D[i][j] = initial_detected_points[i][j] + moving / (cnt + 1e-6);
                if (j == 0
                    && D[i][j].y
                           >= initial_detected_points[i][j].y - y_grid_spacing / 2.0)
                    boarder_nb++;
                if (j == num_grid_rows - 1
                    && D[i][j].y
                           <= initial_detected_points[i][j].y + y_grid_spacing / 2.0)
                    boarder_nb++;
                cost = cost
                       + K1
                             * squared_dist_from_origin(
                                 D[i][j] - initial_detected_points[i][j]
                             );
            }
        }
    }

    if (boarder_nb >= num_grid_rows - 1)
        cost += 1e7;

    for (i = 0; i < num_grid_rows; i++) {
        for (j = 0; j < num_grid_cols; j++) {
            if (occupied[i][j] <= -1) {
                flow1 = D[i][j] - initial_detected_points[i][j];
                for (k = 0; k < 4; k++) {
                    nx = i + dir[k][0];
                    ny = j + dir[k][1];
                    if (nx < 0 || nx > num_grid_rows - 1 || ny < 0
                        || ny > num_grid_cols - 1)
                        continue;
                    if (occupied[nx][ny] > -1) {
                        flow2 =
                            (curr_detected_points[occupied[nx][ny]]
                             - initial_detected_points[nx][ny]);
                        cost += K2 * squared_dist_from_origin(flow2 - flow1);
                    }
                    else if (k < 2 && occupied[nx][ny] <= -1) {
                        flow2 = (D[nx][ny] - initial_detected_points[nx][ny]);
                        cost += K2 * squared_dist_from_origin(flow2 - flow1);
                    }
                }
            }
        }
    }
    return cost;
}

void Point2DMatcher::dfs(
    int i, double cost, int num_missing_markers, int num_extra_markers
)
{
    // if(occupied[6][0] <= -1 && occupied[7][0] <= -1)
    // std::cout<<i<<" "<<"COST: "<<cost<<"fmin: "<< minf<< " missing "<<missing<<"
    // spare "<<spare<<std::endl;
    if (((float)(clock() - most_recent_update_clock_tick)) / CLOCKS_PER_SEC
        >= 1.0 / (double)(this->camera_fps))
        return;

    if (cost >= minf && minf != -1)
        return;

    if (cost >= cost_threshold)
        return;

    int r, k, count = 0, flag, m, same_col;
    double c;
    if (i >= curr_num_detected_points) {
        cost += infer();
        // printf("\nCOST: %lf\n", cost);
        // for (j=0;j<n;j++){
        //     printf("%d %d \t %lf %lf\n", Row[j], Col[j], C[j].x, C[j].y);
        // }
        // printf("--------------------------------------------\n");
        if (cost < minf || minf == -1) {
            // if (int(cost) == 31535) cost = 0;
            minf = cost;
            for (r = 0; r < curr_num_detected_points; r++) {
                // printf("%d %d \t %lf %lf\n", Row[j], Col[j], C[j].x, C[j].y);
                MinRow[r] = Row[r];
                MinCol[r] = Col[r];
                if (Row[r] < 0)
                    continue;
                D[Row[r]][Col[r]].x = curr_detected_points[r].x;
                D[Row[r]][Col[r]].y = curr_detected_points[r].y;
            }
            for (r = 0; r < num_grid_rows; r++) {
                for (k = 0; k < num_grid_cols; k++) {
                    MinOccupied[r][k] = occupied[r][k];
                    MinD[r][k].x = D[r][k].x;
                    MinD[r][k].y = D[r][k].y;
                }
            }
        }
        return;
    }


    for (r = 0; r < i; r++) {
        // if (i == 45) std::cout<<i<<" "<<j<<std::endl;

        if (precessor(i, r)) {
            Row[i] = Row[r];
            Col[i] = Col[r] + 1;
            count++;
            if (Col[i] >= num_grid_cols)
                continue;
            if (occupied[Row[i]][Col[i]] > -1)
                continue;
            if (Row[i] > 0 && occupied[Row[i] - 1][Col[i]] > -1
                && curr_detected_points[i].y
                       <= curr_detected_points[occupied[Row[i] - 1][Col[i]]].y)
                continue;
            if (Row[i] < num_grid_rows - 1 && occupied[Row[i] + 1][Col[i]] > -1
                && curr_detected_points[i].y
                       >= curr_detected_points[occupied[Row[i] + 1][Col[i]]].y)
                continue;
            int vflag = 0;
            for (k = 0; k < num_grid_rows; k++) {
                same_col = occupied[k][Col[i]];
                if (same_col > -1
                    && ((k < Row[i]
                         && curr_detected_points[same_col].y > curr_detected_points[i].y
                        )
                        || (k > Row[i]
                            && curr_detected_points[same_col].y
                                   < curr_detected_points[i].y))) {
                    vflag = 1;
                    break;
                }
            }
            if (vflag == 1)
                continue;
            occupied[Row[i]][Col[i]] = i;

            c = calc_cost(i);
            dfs(i + 1, cost + c, num_missing_markers, num_extra_markers);
            occupied[Row[i]][Col[i]] = -1;
        }
    }


    // if (count == 0) {
    for (r = 0; r < num_grid_rows; r++) {
        if (!done[r]) {
            flag = 0;
            for (int k = 0; k < num_grid_rows; k++) {
                if (done[k]
                    && ((k < r && first[k] > curr_detected_points[i].y)
                        || (k > r && first[k] < curr_detected_points[i].y))) {
                    flag = 1;
                    break;
                }
            }
            if (flag == 1)
                continue;
            done[r] = true;
            first[r] = curr_detected_points[i].y;
            Row[i] = r;
            Col[i] = 0;

            occupied[Row[i]][Col[i]] = i;
            c = calc_cost(i);

            dfs(i + 1, cost + c, num_missing_markers, num_extra_markers);
            done[r] = false;
            occupied[Row[i]][Col[i]] = -1;
        }
    }
    // }

    // considering missing points
    // if (C[i].y > dy && C[i].y < O[0][M-1].y - dy / 2) return;
    for (m = 1; m <= num_missing_markers; m++) {
        for (r = 0; r < num_grid_rows; r++) {
            // if (j >= 1 && j < N - 1) continue;
            if (fabs(curr_detected_points[i].y - initial_detected_points[r][0].y)
                > moving_max)
                continue;
            for (k = num_grid_cols - 1; k >= 0; k--)
                if (occupied[r][k] > -1)
                    break;
            if (k + m + 1 >= num_grid_cols)
                continue;
            if (sqrt(squared_dist_from_origin(
                    curr_detected_points[i] - initial_detected_points[r][k + m + 1]
                ))
                > moving_max)
                continue;
            for (int t = 1; t <= m; t++)
                occupied[r][k + t] = -2;
            Row[i] = r;
            Col[i] = k + m + 1;
            c = calc_cost(i);
            occupied[Row[i]][Col[i]] = i;
            dfs(i + 1, cost + c, num_missing_markers - m, num_extra_markers);

            for (int t = 1; t <= m + 1; t++)
                occupied[r][k + t] = -1;
        }
    }

    if (num_extra_markers > 0) {
        Row[i] = -1;
        Col[i] = -1;
        dfs(i + 1, cost, num_missing_markers, num_extra_markers - 1);
    }
}

void Point2DMatcher::reset_baseline_points() {
    this->is_first_time_step = true;
}

std::tuple<double, double> Point2DMatcher::test()
{
    return std::make_tuple(x_grid_spacing, y_grid_spacing);
}

PYBIND11_MODULE(digit_dot_tracking, m)
{
    py::class_<Point2DMatcher>(m, "Point2DMatcher")
        .def(
            py::init<uint8_t, uint8_t, uint16_t, double, double, double, double>(),
            py::arg("num_grid_rows") = 8U,
            py::arg("num_grid_cols") = 8U,
            py::arg("camera_fps") = 30,
            py::arg("point0_x_coord") = 0.0,
            py::arg("point0_y_coord") = 0.0,
            py::arg("x_grid_spacing") = 10.0,
            py::arg("y_grid_spacing") = 10.0
        )
        .def("match_points", &Point2DMatcher::match_points)
        .def("test", &Point2DMatcher::test)
        .def(
            "update_detected_points", 
            &Point2DMatcher::update_detected_points,
            py::arg("detected_points")
        )
        .def("calc_marker_displacements", &Point2DMatcher::calc_marker_displacements)
        .def("reset_baseline_points", &Point2DMatcher::reset_baseline_points);
}
