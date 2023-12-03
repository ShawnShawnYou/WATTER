

#include "group.h"


Group::Group(ProblemInstance* problem, const unordered_set<int>& order_ids) {
    // 0. init
    capacity = order_ids.size();
    driver_serve_time = 0;

    if (order_ids.size() == 1) {
        driver_serve_time = problem->get_single_order(*order_ids.begin()).get_cost_time();
    }

    for (auto& id : order_ids) {
        ids.insert(id);
        orders_durable.push_back(problem->get_single_order(id).get_durable());
        speed = problem->get_single_order(id).get_speed();
    }

    is_single = (order_ids.size() == 1);
}

double Group::get_order_total_cost(ProblemInstance* problem) const {
    double ret = 0;
    for (int id : ids)
        ret += problem->get_single_order(id).get_cost_time();
    return ret;
}


double Group::get_order_total_slack(ProblemInstance* problem) const {
    double ret = 0;
    for (int id : ids)
        ret += problem->get_single_order(id).get_original_slack();
    return ret;
}


double Group::get_total_extra_response_time() const {
    double ret = 0;
    for (auto i : extra_response_time) {
        ret += i;
    }
    return ret;
}


double Group::get_total_extra_detour_time() const {
    double ret = 0;
    for (auto i : extra_detour_time) {
        ret += i;
    }
    return ret;
}

double Group::get_avg_extra_waiting_time() const {
    double total_extra_waiting_time = get_total_extra_response_time() + get_total_extra_detour_time();
    return total_extra_waiting_time / capacity;
}


double Group::get_total_order_cost() const {
    double ret = 0;
    for (auto i : order_cost) {
        ret += i;
    }
    return ret;
}

void Group::get_extra_time(vector<double>& tmp) {
    tmp.resize(0);
    for (int i = 0; i < extra_response_time.size(); i++) {
        tmp.emplace_back(extra_response_time[i] + extra_detour_time[i]);
    }
}