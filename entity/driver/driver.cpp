

#include "driver.h"


Driver::Driver(int did, pair<double, double> location) {
    id = did;
    location_x = location.first;
    location_y = location.second;

    capacity = CAPACITY_CONSTRAINT; // todo random
    capacity = rand() % (CAPACITY_CONSTRAINT - 1) + 2;
//    capacity = rand() % CAPACITY_CONSTRAINT + 1;

    start_time_list.emplace_back(0);
    end_time_list.emplace_back(0);

    int start_gid = -1;
    in_which_grid_id(location_x, location_y, start_gid);
    history_gid.push_back(start_gid);
    history_driver_cost.emplace_back(0);
    history_payment.emplace_back(0);
    history_pick_cost.emplace_back(0);
    history_num.emplace_back(0);

    for (int i = 0; i <= CAPACITY_CONSTRAINT; i++)
        serve_num_time_map[i] = 0;

}








