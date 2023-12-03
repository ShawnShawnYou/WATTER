#ifndef TEST_GLOBAL_VARIABLES_H
#define TEST_GLOBAL_VARIABLES_H

#include "../entity/shareability_graph/shareability_graph_rewrite.h"


// envs
extern ShareabilityGraph* sg;

// oracle
extern unordered_map<int, double> expect_min_time;
extern unordered_map<int, int> expect_round;
extern unordered_map<int, int> policy_map;


// time
extern time_t algorithm_start_time;
extern time_t batch_time;
extern time_t algorithm_finish_time;


// tmp and local var
extern pair<int, vector<double>> tmp;
extern int batch_round;
extern int dispatch_section;
extern bool is_new_timeslot;
extern bool is_policy;

// result
extern int count_1, count_2, count_3, count_4, count_5;
extern int count_all;
extern result_t result; // map<order, pair<partner, order.revenue>>


// draw data
extern unordered_map<int, vector<double>*> tr_map;
extern unordered_map<int, vector<double>*> td_map;
extern unordered_map<int, vector<double>*> te_map;
extern vector<double> on_policy_residence;
extern vector<double> off_policy_residence;




#endif //TEST_GLOBAL_VARIABLES_H