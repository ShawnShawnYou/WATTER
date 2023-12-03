#ifndef E_RIDE_TEST_GLOBAL_VARIABLE_H
#define E_RIDE_TEST_GLOBAL_VARIABLE_H

#include <bits/stdc++.h>
#include "torch/torch.h"
#include <random>
using namespace std;

typedef unordered_map<int, pair<int, vector<double>>> result_t;
typedef unsigned int uint;

extern int FRAGMENT;
extern int TIMESLOT_DELTA;
extern double DEADLINE_GAMMA;
extern int WORKER_NUM;
extern bool TRAIN_UNDER_EXPECT;
extern int MODE;
extern int TRAIN_ROUND;
extern bool USE_DISTRIBUTION_PREDICT;
extern bool TRAIN;
extern float CHECK_TRUE_POSSIBILITY;
extern string city;
extern string algo_name;
extern string MODEL_TRAIN_ALGO;
extern bool USE_TARGET_VALUE;
extern uint CAPACITY_CONSTRAINT;
extern int REQUEST_SIZE;
extern bool REQUEST_CHECK;
extern double UNI_SPEED;
extern double COMPARE_SPEED;
extern double DURABLE_BASE;
extern int NUM_GRID_X;
extern int NUM_GRID_Y;
extern int GRID_SIZE;

extern pair<double, double> XREGION;
extern pair<double, double> YREGION;


const double DETOUR_RATE = 1;
const double CR = 2;
const double CW = 1;
const double ALPHA = 1;
const double PENALTY = 10;

const int INF_WEIGHT = numeric_limits<int>::max() / 3;
const double EPS = 1e-6;
const double INF = INF_WEIGHT;

const bool USE_MODEL = true;
const int CHECK_ROUND = 1;
const int WATCHING_WINDOW_SIZE = 5;




#endif //E_RIDE_TEST_GLOBAL_VARIABLE_H



