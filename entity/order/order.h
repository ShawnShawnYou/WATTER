#ifndef E_RIDE_TEST_ORDER_H
#define E_RIDE_TEST_ORDER_H

#include "../../util/setting.h"
#include <bits/stdc++.h>
using namespace std;


#ifdef __linux__
static const string ORDER_DATA_DIR = "";
static const string ORDER_DATA_FILE = "";
#elif _WIN32
static const string ORDER_DATA_DIR = R"";
static const string ORDER_DATA_FILE = "";
#endif

class Order {
private:
    int id;

    double speed;

    time_t pick_time;

    int passenger_count = 1;

    double pick_x;
    double pick_y;
    double drop_x;
    double drop_y;

    double absolute_distance;
    time_t deadline;



public:
    Order();
    explicit Order(vector<string> order_str, time_t start_time);
    Order(int order_id, time_t pick_time, pair<double, double> pick_location, pair<double, double> drop_location);
    void recompute_ddl();
    int get_id() const { return this->id; }
    void set_id(int order_id) { id = order_id; }

    time_t get_pick_time() const { return this->pick_time; }
    void set_pick_time(time_t pick_t) { pick_time = pick_t; recompute_ddl(); }
    time_t get_cost_time() const { return (this->absolute_distance / this->speed); }
    time_t get_ddl() const { return this->deadline; }
    time_t get_original_slack() const { return this->deadline - this->pick_time - this->get_cost_time(); }
    int get_passenger_count() const { return this->passenger_count; }

    double get_speed() const { return this->speed; }
    int get_durable();

    double get_absolute_distance() const { return this->absolute_distance; }


    pair<double, double> get_pick_location() const {return make_pair(this->pick_x, this->pick_y); }
    pair<double, double> get_drop_location() const {return make_pair(this->drop_x, this->drop_y); }
};

#endif //E_RIDE_TEST_ORDER_H

