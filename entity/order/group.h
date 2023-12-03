#ifndef TEST_GROUP_H
#define TEST_GROUP_H


#include "../grid/grid.h"
#include "../../util/setting.h"
#include "../problem/problem.h"

using namespace std;

class Group {
private:

    int o1_id;
    int o2_id;
    unordered_set<int> ids;

    bool is_single;
    int capacity;

    pair<double, double> start_position;
    pair<double, double> end_position;
    vector<int> orders_durable;

    int start_grid_id;
    int end_grid_id;

    int how_dispatch;
    int expire_timeslot;
    double min_slack_time;

    vector<double> group_attr;
    vector<double> extra_response_time;
    vector<double> extra_detour_time;
    vector<double> order_cost;

    time_t grouped_time; //
    double driver_serve_time;   // 从driver接到，到完成订单的时间（没有算delay和pick time）
    double pick_cost;
    double revenue;
    double speed;

public:
    unordered_map<int, double> response_time;
    unordered_map<int, double> detour_time;
    unordered_map<int, double> extra_time;
    unordered_map<int, double> expected_time;

    Group() {};
    Group(ProblemInstance* problem, const unordered_set<int>& order_ids);

    int get_start_grid_id() { return start_grid_id; }
    int get_end_grid_id() { return end_grid_id; }
    void set_start_grid_id(int start_id) { start_grid_id = start_id; }
    void set_end_grid_id(int end_id) { end_grid_id = end_id; }

    pair<double, double> get_start_position() const { return start_position; }
    pair<double, double> get_end_position() const { return end_position; }
    void set_start_position(pair<double, double> position) { start_position = {position.first, position.second}; }
    void set_end_position(pair<double, double> position) { end_position = {position.first, position.second}; }

    const unordered_set<int>& get_member_ids() const { return ids; }
//    unordered_set<int> get_member_ids() { return ids; }
    const vector<int>& get_orders_durable() const { return orders_durable; }

    int get_capacity() const { return capacity; }
    double get_driver_cost() const { return driver_serve_time; }
    void set_driver_cost(double cost) { driver_serve_time = cost; }
    void add_time_to_driver_cost(double driver_to_pick_time) { driver_serve_time += driver_to_pick_time; }
    double get_pick_cost() const { return pick_cost; }
    void set_pick_cost(double cost) { pick_cost = cost; }

    time_t get_grouped_time() const { return grouped_time; }
    void set_grouped_time(time_t g_tim) { grouped_time = g_tim; }

    double get_order_total_cost(ProblemInstance* problem) const;
    double get_order_total_slack(ProblemInstance* problem) const;
    double get_total_extra_response_time() const;
    double get_total_extra_detour_time() const;
    double get_total_order_cost() const;
    double get_avg_extra_waiting_time() const;
    double get_speed() { return speed; };

    const int get_how_dispatch() const { return how_dispatch; }

    void get_extra_time(vector<double>& tmp);
    void add_response_time(double r_tim) { extra_response_time.push_back(r_tim); }
    void add_detour_time(double d_tim) { extra_detour_time.push_back(d_tim); }
    void add_order_cost(double cost) { order_cost.push_back(cost); }
    void clear_response_time() { extra_response_time.clear(); }
    void clear_detour_time() { extra_detour_time.clear(); }
    void clear_order_cost() { order_cost.clear(); }

//    bool update_group_attr(ProblemInstance* problem, time_t now_time);
    vector<double>& get_group_attr() { return group_attr; }

    void set_how_dispatch(int dispatch_section) { how_dispatch = dispatch_section; }
    void set_expire_timeslot(int timeslot) { expire_timeslot = timeslot; }
    int get_expire_timeslot() { return expire_timeslot; }
    void set_min_slack_time(double slack_time) { min_slack_time = slack_time; }
    double get_min_slack_time() { return min_slack_time; }

};


#endif //TEST_GROUP_H
