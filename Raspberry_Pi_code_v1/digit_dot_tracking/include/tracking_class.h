#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <thread>

namespace py = pybind11;

typedef std::vector<double> vd;
typedef std::vector<std::vector<double>> vvd;

#define MAX_NUM_ROWS   16
#define MAX_NUM_COLS   16
#define MAX_NUM_POINTS 255

/**
 * @brief Represents a real-valued point in two dimensions
 */
struct Point2D
{
    double x, y;

    // TODO: Allow user to configure number max number of markers.
    // `uint16_t` assumes no more then 65536 markers
    uint16_t id;

    Point2D(): x(0.0), y(0.0), id(0) {}

    Point2D(double x, double y, uint32_t id = 0): x(x), y(y), id(id) {}

    bool operator<(const Point2D& other) const
    {
        return (x < other.x || (x == other.x && y < other.y));
    }

    Point2D operator- (const Point2D& rhs) const { return {x - rhs.x, y - rhs.y}; }

    Point2D operator+ (const Point2D& rhs) const { return {x + rhs.x, y + rhs.y}; }

    Point2D operator/ (const double divisor) const
    {
        return {x / divisor, y / divisor};
    }
};

class Point2DMatcher {
    /**
     *  A class that helps keeps track of labeled 2D points in a structured rectangular
     *  grid as the points vary in position between time steps.
     */

    private:
    // double x_0 = 160, y_0 = 30, dx = 43.0, dy = 43.0; //GelSight Hanjun x1
    // double x_0 = 34, y_0 = 37, dx = 27.0, dy = 27.0; //34 - 223,  37 - 200
    // GelSight_SX double x_0 = 6, y_0 = 16, dx = 31.0, dy = 31.0; //6 - 130,  16
    // - 138 HSR x0.5 double x_0 = 12, y_0 = 32, dx = 62.0, dy = 62.0; //6 - 130,
    // 16 - 138 HSR x1 double x_0 = 15, y_0 = 15, dx = 23.0, dy = 23.0; //15-195
    // 15-202 HSR blue

    int Row[MAX_NUM_POINTS], Col[MAX_NUM_POINTS];

    int occupied[MAX_NUM_ROWS][MAX_NUM_COLS], first[MAX_NUM_ROWS];
    uint16_t camera_fps;

    double dmin, dmax, theta;
    double moving_max;
    double cost_threshold, flow_difference_threshold;

    // Dynamic point tracking information (updated between time steps)
    clock_t most_recent_update_clock_tick;
    bool is_first_time_step;

    double curr_detected_point_pairwise_squared_dists[MAX_NUM_POINTS][MAX_NUM_POINTS];
    double curr_detected_point_pairwise_angles[MAX_NUM_POINTS][MAX_NUM_POINTS];

    uint16_t curr_num_detected_points;

    Point2D curr_detected_points[MAX_NUM_POINTS];
    Point2D initial_detected_points[MAX_NUM_ROWS][MAX_NUM_COLS];

    bool done[MAX_NUM_ROWS];

    // Static point tracking information (constant across all time steps)
    const uint8_t num_grid_rows, num_grid_cols;
    const uint16_t actual_num_points;
    const double x_grid_spacing, y_grid_spacing;

    public:
    int MinRow[MAX_NUM_POINTS], MinCol[MAX_NUM_POINTS],
        MinOccupied[MAX_NUM_ROWS][MAX_NUM_COLS];
    double minf = -1;

    Point2D D[MAX_NUM_ROWS][MAX_NUM_COLS];
    Point2D MinD[MAX_NUM_ROWS][MAX_NUM_COLS];

    double K1 = 0.1, K2 = 1;

    Point2DMatcher(
        uint8_t num_grid_rows = 8U,
        uint8_t num_grid_cols = 8U,
        uint16_t camera_fps = 30U,
        double point0_x_coord = 0.0,
        double point0_y_coord = 0.0,
        double x_grid_spacing = 10.0,
        double y_grid_spacing = 10.0
    );

    void update_detected_points(std::vector<std::vector<double>> detected_points);
    void match_points();
    void reset_baseline_points();

    int precessor(int i, int j);
    double calc_cost(int i);
    void dfs(int i, double cost, int num_missing_markers, int num_extra_markers);
    std::tuple<vvd, vvd, vvd, vvd, vvd> calc_marker_displacements();
    std::tuple<double, double> test();
    double infer();
};
