#ifndef TEST_E_PARTIAL_H
#define TEST_E_PARTIAL_H

#include "value_distribution.h"
#include <gsl/gsl_histogram.h>

void init_variables();

bool predict_process(unordered_set<int>& working_set);

bool make_decision_rewrite(int exist_order_id, bool& is_policy);

pair<double, double> get_best_value(int exist_order_id);

bool dispatch_to_be_served_group();

void show_result_tmp();

void show_result();

void show_result_by_group_list();

void show_driver_states();

void show_distribution();

#endif //TEST_E_PARTIAL_H