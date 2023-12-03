#include "grid_index.h"

GridIndex::GridIndex() {
    for (int i = 0; i < NUM_GRID_X; i++)
        for (int j = 0; j < NUM_GRID_Y; j++)
            grid_index.emplace_back(i * NUM_GRID_Y + j);
}


bool GridIndex::insert_into_grid(const Order &order) {
    int gid = -1;
    in_which_grid_id(order.get_pick_location().first, order.get_pick_location().second, gid);
    grid_index[gid].insert_order(order.get_id());
    grid_map[order.get_id()] = gid;
    return true;
}


bool GridIndex::remove_from_grid(const Order &order) {
    if (grid_map.count(order.get_id()) == 0)
        return false;
    int gid = grid_map[order.get_id()];
    grid_index[gid].remove_order(order.get_id());
    grid_map.erase(order.get_id());
    return true;
}


bool GridIndex::find_grid_by_order_id(const int &order_id, Grid** grid_ptr) {
    if (grid_map.count(order_id) == 0)
        return false;
    *grid_ptr = &grid_index[grid_map[order_id]];
    return true;
}


bool GridIndex::add_drivers(const vector<Driver> &all_drivers) {
    int gid = -1;
    for (const auto& driver : all_drivers) {
        in_which_grid_id(driver.get_location().first, driver.get_location().second, gid);
        grid_index[gid].insert_driver(driver.get_id());
    }
    return true;
}


bool GridIndex::get_neighbor_grids(int gid, vector<int>& neighbor_grid_id) {
    vector<int> tmp_gids = {gid + 1, gid, gid - 1,
                            gid + NUM_GRID_Y, gid + NUM_GRID_Y + 1, gid + NUM_GRID_Y - 1,
                            gid - NUM_GRID_Y, gid - NUM_GRID_Y + 1, gid - NUM_GRID_Y - 1};
    vector<int> ret;
    for (auto gid : tmp_gids) {
        if ((0 <= gid) and (gid < grid_index.size()))
            ret.push_back(gid);
    }

    neighbor_grid_id.swap(ret);

    return false;
}


bool GridIndex::get_nearest_available_driver_in_neighbors(ProblemInstance* problem, Group& group, time_t now_time,
                                                          int& driver_id, int& gid,
                                                          double& min_wait_delay_time, double& min_wait_pick_time) {
    int min_driver_id;
    double tmp_wait_delay_time = INF, tmp_wait_pick_time = INF, tmp_cost_time = INF;
    double min_finish_time = INF;
    vector<int> neighbor_grids;
    bool ret = false;

    get_neighbor_grids(group.get_start_grid_id(), neighbor_grids);
    for (auto& iter : neighbor_grids) {
        unordered_set<int>& driver_set = grid_index[iter].get_driver_set();

        for (const auto& tmp_driver_id : driver_set) {
            Driver& driver = problem->get_single_driver(tmp_driver_id);
            if (is_accept(problem, driver, group, now_time,
                                 tmp_wait_delay_time, tmp_wait_pick_time,
                                 tmp_cost_time)) {
                if (tmp_wait_delay_time + tmp_wait_pick_time < min_finish_time) {
                    min_wait_delay_time = tmp_wait_delay_time;
                    min_wait_pick_time = tmp_wait_pick_time;
                    min_driver_id = tmp_driver_id;
                    gid = iter;
                }
                ret = true;
            }
        }
    }
    driver_id = min_driver_id;
    return ret;

}
