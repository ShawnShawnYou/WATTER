#ifndef TEST_GRID_INDEX_H
#define TEST_GRID_INDEX_H
#include "../../util/setting.h"
#include "../problem/problem.h"
#include "../order/order.h"
#include "../driver/driver.h"
#include "grid.h"
#include "../../util/util_grid.h"
#include "../../util/util.h"

class GridIndex {
private:

    vector<Grid> grid_index;

    unordered_map<int, int> grid_map;

    int max_grid_id = NUM_GRID_X * NUM_GRID_Y - 1;

public:

    GridIndex();

    Grid& get_single_grid(int gid) {
        if (0 <= gid and gid <= max_grid_id)
            return grid_index[gid];
        else if (gid > max_grid_id) {
            grid_index[max_grid_id];
        } else {
            return grid_index[0];
        }
    }

    int get_grid_id_by_order_id(int order_id) { return grid_map[order_id]; };

    bool insert_into_grid(const Order& order);

    bool remove_from_grid(const Order& order);

    bool find_grid_by_order_id(const int& order_id, Grid** grid_ptr);

    bool add_drivers(const vector<Driver>& all_drivers);

    bool get_neighbor_grids(int gid, vector<int>& neighbor_grid_id);

    bool get_nearest_available_driver_in_neighbors(ProblemInstance* problem, Group& group, time_t now_time,
                                                   int& driver_id, int& gid,
                                                   double& min_wait_delay_time, double& min_wait_pick_time);
};
#endif //TEST_GRID_INDEX_H