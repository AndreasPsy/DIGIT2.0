from __future__ import annotations

import numpy as np

class Point2DMatcher:
    def __init__(
        self, 
        num_grid_rows: np.int8, 
        num_grid_cols: np.int8,
        camera_fps: np.int16,
        point0_x_coord: float,
        point0_y_coord: float,
        x_grid_spacing: float,
        y_grid_spacing: float
    ) -> None: ...

    def match_points(self) -> None: ...

    def test(self) -> None: ...

    def update_detected_points(self, detected_points: list[list[float]]) -> None: ...

    def calc_marker_displacements(self) -> tuple[
        list[float], list[float], list[float], list[float], list[float]
    ]: ...

    def reset_baseline_points(self) -> None: ...