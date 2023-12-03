#ifndef E_RIDE_VALUE_DISTRIBUTION
#define E_RIDE_VALUE_DISTRIBUTION

#include "../util/setting.h"
#include "global_variables.h"

void predict_by_ground_truth(unordered_map<int, double>& expect_min_time, unordered_map<int, int>& expect_round);

#endif //E_RIDE_VALUE_DISTRIBUTION