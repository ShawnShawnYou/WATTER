#ifndef TEST_DRIVER_H
#define TEST_DRIVER_H

#include "../../util/setting.h"
#include "../util/util_grid.h"

class Driver {
private:
    int id;

    vector<int> served_order;

    double location_x;
    double location_y;

    int capacity;




public:
    Driver(int did, pair<double, double> location);

    vector<time_t> start_time_list;
    vector<time_t> end_time_list;

    vector<int> history_gid;
    vector<double> history_pick_cost;
    vector<double> history_driver_cost;
    vector<double> history_payment;
    vector<int> history_num;

    unordered_map<int, double> serve_num_time_map;

    void add_num_serve(int k, double v) { serve_num_time_map[k] += v; }
    double get_serve_num(int k) const { return serve_num_time_map.at(k); }

    int get_id() const { return id; }

    int get_num_served() const { return start_time_list.size() - 1;}

    pair<double, double> get_location() const { return make_pair(this->location_x, this->location_y); }
    void set_location(double x, double y) { location_x = x; location_y = y; }
    int get_capacity() const { return capacity; }





};

#endif //TEST_DRIVER_H
