
#include "util_grid.h"


bool in_which_grid(double x, double y, pair<int, int>& ret) {
    double single_x = (XREGION.second - XREGION.first) / (NUM_GRID_X);
    double single_y = (YREGION.second - YREGION.first) / (NUM_GRID_Y);

    ret.first = floor((x - XREGION.first) / single_x);
    ret.second = floor((y - YREGION.first) / single_y);

    return true;
}


bool in_which_grid_id(double x, double y, int& gid) {
    pair<int, int> tmp;
    in_which_grid(x, y, tmp);
    int x_coordinate = tmp.first;
    int y_coordinate = tmp.second;
    gid = x_coordinate * NUM_GRID_Y + y_coordinate;
    if (gid >= NUM_GRID_X * NUM_GRID_Y)
        gid = NUM_GRID_X * NUM_GRID_Y - 1;
    return true;
}