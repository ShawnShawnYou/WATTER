

#ifndef E_RIDE_TEST_UTIL_H
#define E_RIDE_TEST_UTIL_H

#include "setting.h"
#include "../entity/problem/problem.h"
#include "../entity/order/group.h"
#include "util_grid.h"
#include <sys/time.h>
//#include <time.h>
//#include <Python.h>

struct Worker {
    pair<double, double> position;
    int num, cap;
    time_t tim;
    vector<int> S;
    vector<time_t> reach;
    vector<int> picked;
    vector<double> slack;

    inline void pop() {
        if (!S.empty()) {
            S.erase(S.begin());
            reach.erase(reach.begin());
            picked.erase(picked.begin());
            slack.erase(slack.begin());
        }
    }
};

struct Request {
    int id;
    pair<double, double> s;
    pair<double, double> e;
    time_t tim;
    double r_tim;
    int com;
    double ddl, len;
};

double cost_time_between(pair<double, double>& n1, pair<double, double>& n2);

bool cost_saving(Order o1, Order o2, vector<double>& o1_attr, vector<double>& o2_attr, vector<double>& total_attr);

bool is_matchable(Order o1, Order o2, time_t now_time, vector<double>& o1_attr, vector<double>& o2_attr, vector<double>& total_attr);

bool compute_group_attr(const Order& o1, const Order& o2, time_t now_time, vector<double>& attr, vector<double>& o1_attr, vector<double>& o2_attr);

void save_features(vector<vector<int>>& features);

void model(vector<int>& predict_result);

template<typename T>
bool load_map(unordered_map<int, T>& the_map, const string& file_path);

template<typename T>
bool save_map(const unordered_map<int, T>& the_map, const string& file_path);



long nowTime();

pair<double, double> Pos(int x, unordered_map<int, Request>& R);

void insertion(Worker& w, Request& r, unordered_map<int, Request>& R);

void try_insertion(Worker &w_origin, Request& r, unordered_map<int, Request>& R, double &delta);

void try_insertion_euclid(Worker& w, Request& r, unordered_map<int, Request>& R, double &delta);

void updateDriverArr(Worker &w, unordered_map<int, Request>& R);

void insertion(Worker& w, Request& r, unordered_map<int, Request>& R);

bool routing(ProblemInstance* problem, Group& group, time_t now_time, double delay_pick, double& min_slack_time, double& cost_time);

bool is_accept(ProblemInstance* problem, Driver& driver, Group& group, time_t now_time,
               double& wait_delay_time, double& driver_to_pick_time,
               double& cost_time);

bool serve(ProblemInstance* problem, Driver& driver, Group& group, time_t now_time, time_t pick_time);

#endif //E_RIDE_TEST_UTIL_H
