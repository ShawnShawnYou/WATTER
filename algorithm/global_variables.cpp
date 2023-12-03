#include "global_variables.h"


// envs
ShareabilityGraph* sg = nullptr;
ProblemInstance* problem = nullptr;
GridIndex* gi = nullptr;
DQNAgent* dqn = nullptr;

// oracle
unordered_map<int, double> expect_min_time;
unordered_map<int, int> expect_round;
unordered_map<int, int> policy_map;


// time
time_t algorithm_start_time;
time_t batch_time;
time_t algorithm_finish_time;


// tmp and local var
pair<int, vector<double>> tmp;
int batch_round = 0;
int dispatch_section = 0;
bool is_new_timeslot = true;
bool is_policy = false;

// result
int count_1 = 0, count_2 = 0, count_3 = 0, count_4 = 0, count_5 = 0;
int count_all = 0;
result_t result;


// draw data
unordered_map<int, vector<double>*> tr_map;
unordered_map<int, vector<double>*> td_map;
unordered_map<int, vector<double>*> te_map;
vector<double> on_policy_residence(NUM_GRID_X * NUM_GRID_Y);
vector<double> off_policy_residence(NUM_GRID_X * NUM_GRID_Y);





