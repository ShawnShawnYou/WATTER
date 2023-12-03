#include "util.h"

double cost_time_between(pair<double, double>& n1, pair<double, double>& n2) {
    double absolute_distance = abs(n1.first - n2.first) + abs(n1.second - n2.second);
    return (absolute_distance) / UNI_SPEED;
}


bool is_matchable(Order o1, Order o2, time_t now_time, vector<double>& o1_attr, vector<double>& o2_attr, vector<double>& total_attr) {
    if (cost_saving(o1, o2, o1_attr, o2_attr, total_attr)) {
        if (now_time + o1_attr[3] < o1.get_ddl() and
            now_time + o2_attr[3] < o2.get_ddl())
            return true;
    }
    return false;
}


long nowTime(){
    struct timeval curTime{};
    gettimeofday(&curTime, nullptr);
    long milli = curTime.tv_usec  + curTime.tv_sec * 1000 * 1000;
    return milli;
}


bool cost_saving(Order o1, Order o2, vector<double>& o1_attr, vector<double>& o2_attr, vector<double>& total_attr) {
    auto L1_pick2pick = [](Order o1, Order o2) -> double {
        return abs(o1.get_pick_location().first - o2.get_pick_location().first) +
        abs(o1.get_pick_location().second - o2.get_pick_location().second);
    };

    auto L1_pick2drop = [](Order o1, Order o2) -> double {
        return abs(o1.get_pick_location().first - o2.get_drop_location().first) +
        abs(o1.get_pick_location().second - o2.get_drop_location().second);
    };

    auto L1_drop2drop = [](Order o1, Order o2) -> double {
        return abs(o1.get_drop_location().first - o2.get_drop_location().first) +
        abs(o1.get_drop_location().second - o2.get_drop_location().second);
    };

    double v1x = o1.get_drop_location().first - o1.get_pick_location().first;
    double v2x = o2.get_drop_location().first - o2.get_pick_location().first;
    double v1y = o1.get_drop_location().second - o1.get_pick_location().second;
    double v2y = o2.get_drop_location().second - o2.get_pick_location().second;

    double cos_theta = (v1x * v2x + v1y * v2y) / (sqrt(v1x * v1x + v1y * v1y) * sqrt(v2x * v2x + v2y * v2y));
//    if (cos_theta > 1) { cos_theta = 1; }
//    if (cos_theta < -1) { cos_theta = -1; }

    double theta = acos(cos_theta) * 57.3;

    if (theta < 90 and
    L1_pick2pick(o1, o2) / o1.get_speed() < o1.get_durable() and
    L1_pick2pick(o1, o2) / o2.get_speed() < o2.get_durable()) {

        double common = L1_pick2pick(o1, o2) + L1_drop2drop(o1, o2);

        double plan1 = common + L1_pick2drop(o2, o1);
        double plan2 = common + L1_pick2drop(o2, o2);
        double plan3 = common + L1_pick2drop(o1, o1);
        double plan4 = common + L1_pick2drop(o1, o2);

        double share = min(min(plan1, plan2), min(plan3, plan4));
        double total_time = share * 2 / (o1.get_speed() + o2.get_speed());

        double detour_time_i, detour_time_j, cost_time_i, cost_time_j;
        double ci = 0;
        if (share == plan1) {
            ci = L1_pick2pick(o1, o2) + L1_pick2drop(o2, o1) / 2;
            cost_time_i = (L1_pick2pick(o1, o2) + L1_pick2drop(o2, o1)) / o1.get_speed();
            cost_time_j = total_time;
        }
        if (share == plan2) {
            ci = common + L1_pick2drop(o2, o2) / 2;
            cost_time_i = total_time;
            cost_time_j = (L1_pick2pick(o1, o2) + L1_pick2drop(o2, o2)) / o2.get_speed();
        }
        if (share == plan3) {
            ci = L1_pick2drop(o1, o1) / 2;
            cost_time_i = (L1_pick2pick(o1, o2) + L1_pick2drop(o1, o1)) / o1.get_speed();
            cost_time_j = total_time;
        }
        if (share == plan4) {
            ci = L1_pick2drop(o1, o2) / 2 + L1_drop2drop(o2, o1);
            cost_time_i = total_time;
            cost_time_j = (L1_pick2pick(o2, o1) + L1_pick2drop(o1, o2)) / o2.get_speed();
        }
        detour_time_i = cost_time_i - L1_pick2drop(o1, o1) / o1.get_speed();    // cost_time = 司机起点到完成
        detour_time_j = cost_time_j - L1_pick2drop(o2, o2) / o2.get_speed();

        double start_order_id, end_order_id;
        start_order_id = (share == plan1 or share == plan2)? o1.get_id() : o2.get_id();
        end_order_id = (share == plan1 or share == plan3)? o1.get_id() : o2.get_id();

        double d_sum = o1.get_absolute_distance() + o2.get_absolute_distance();

        if (d_sum == 0) {
            return false;
        } else {
            double rate_1, rate_2, save_individual_1, save_individual_2, save_total;
            rate_1 = o1.get_absolute_distance() / d_sum;
            rate_2 = o2.get_absolute_distance() / d_sum;
            save_individual_1 = (o1.get_absolute_distance() - ci) / o1.get_absolute_distance();
            save_individual_2 = (o2.get_absolute_distance() - (share - ci)) / o2.get_absolute_distance();
            save_total = d_sum - share;

            o1_attr = {rate_1, save_individual_1, detour_time_i, cost_time_i};
            o2_attr = {rate_2, save_individual_2, detour_time_j, cost_time_j};
            total_attr = {save_total, total_time, start_order_id, end_order_id};
            return true;
        }
    }
    return false;
}


void save_features(vector<vector<int>>& features) {
    const string FEATURE_PATH = "../file/features.txt";
    const string PREDICT_RESULT_PATH = "../file/results.txt";

    ofstream outfile;
    outfile.open(FEATURE_PATH);
    for (const auto& feature : features) {
        for (const auto& iter : feature)
            outfile << iter << " ";
        outfile << endl;
    }
    outfile.close();
}


void model(vector<int>& predict_result) {
    const string FEATURE_PATH = "../file/features.txt";
    const string PREDICT_RESULT_PATH = "../file/results.txt";
    // boot model
//    Py_SetPythonHome(L"D:/ProgramData/Anaconda3/envs/rl_exp");
//    Py_Initialize();
//    PyRun_SimpleString("import sys");
//    PyRun_SimpleString("import os");
//    PyRun_SimpleString("os.system('python E:/workspace/A-Experiment-code/FORM_Experiment_Code/ml4rs_poc/experiment/cost_saving_increase_data/classfier.py')");
//    Py_Finalize();
    system("python classfier.py");
    // read local file
    ifstream fin;
    fin.open(PREDICT_RESULT_PATH);
    int tmp;
    while (fin >> tmp)
        predict_result.push_back(tmp);
}



template bool load_map<double>(unordered_map<int, double>& the_map, const string& file_path);
template bool load_map<int>(unordered_map<int, int>& the_map, const string& file_path);
template bool save_map<int>(const unordered_map<int, int>& the_map, const string& file_path);
template bool save_map<double>(const unordered_map<int, double>& the_map, const string& file_path);


template<typename T>
bool save_map(const unordered_map<int, T>& the_map, const string& file_path) {
    try {
        ofstream outfile;
        outfile.open(file_path);
        for (auto iter = the_map.begin(); iter != the_map.end(); iter++)
            outfile << iter->first << " " << iter->second << endl;
        outfile.close();
        return true;
    } catch (exception& e) {
        return false;
    }
}


template<typename T>
bool load_map(unordered_map<int, T>& the_map, const string& file_path) {
    try {
        bool valid_flag = false;
        ifstream fin;
        fin.open(file_path);
        if (!fin.is_open()) return false;

        string line;
        while (getline(fin, line)) {
            stringstream ss(line);
            string key, value;
            getline(ss, key, ' ');
            getline(ss, value, ' ');
            the_map[stoi(key)] = (T) stod(value);
            valid_flag = true;
        }
        fin.close();
        return valid_flag;
    } catch (exception& e) {
        return false;
    }
}




bool compute_group_attr(const Order& o1, const Order& o2, time_t now_time, vector<double>& attr,
                        vector<double>& o1_attr, vector<double>& o2_attr) {
    vector<double> all_attr(4);

    try {
        if (is_matchable(o1, o2, now_time, o1_attr, o2_attr, all_attr)) {
            for (int i = 0; i < 3; i++)
                attr[i] = o1_attr[i] + o2_attr[i];
        }
        else {
            for (int i = 0; i < 3; i++) attr[i] = 0;
            return false;
        }

    } catch (exception& e) {
        return false;
    }

    attr[2] *= DETOUR_RATE;
    attr[2] += (double)(now_time - o1.get_pick_time());
    attr[2] += (double)(now_time - o2.get_pick_time());


    return true;
}


pair<double, double> Pos(int x, unordered_map<int, Request>& R) {
    return (x & 1) ? R[x >> 1].e : R[x >> 1].s;
}

void try_insertion(Worker &w_origin, Request& r, unordered_map<int, Request>& R, double &delta) {
    Worker w(w_origin);
    double opt = INF;

    delta = INF;
    if (w.S.empty()) {
        double tmp = w.tim + cost_time_between(w.position, r.s) + r.len;
        if (tmp < r.ddl + EPS && r.com <= w.cap) {
            opt = tmp - w.tim;
            delta = opt;
        }
        return;
    }

    // local cache of distances
    double swd = cost_time_between(w.position, r.s);
    vector<double> sdcache(w.S.size(), INF);
    vector<double> edcache(w.S.size(), INF);
    for (int i = 0; i < w.S.size(); ++i) {
        pair<double, double> tmp11111 = Pos(w.S[i], R);
        pair<double, double> tmp22222 = Pos(w.S[i], R);
        sdcache[i] = cost_time_between(r.s, tmp11111);
        edcache[i] = cost_time_between(r.e, tmp22222);
    }

    const vector<int> &picked = w.picked;
    const vector<double> &slack = w.slack;
    const vector<time_t> &reach = w.reach;

    for (int i = 0; i <= w.S.size(); ++i) {
        if (i == 0) {
            double part1 = swd;
            double detour = part1 + r.len + edcache[i] - (reach[0] - w.tim);
            if (w.num + r.com <= w.cap &&
            detour < slack[i] + EPS &&
            part1 + r.len < r.ddl - w.tim + EPS) {
                if (opt > detour) {
                    opt = detour;
                    delta = opt;
                }
            }
        } else if (i == w.S.size()) {
            double detour = sdcache[i - 1] + r.len;
            if (picked[i - 1] + r.com <= w.cap &&
            detour < r.ddl - reach[i - 1] + EPS) {
                if (opt > detour) {
                    opt = detour;
                    delta = opt;
                }
            }
        } else {
            double part1 = sdcache[i - 1];
            double detour = part1 + r.len + edcache[i] - (reach[i] - reach[i - 1]);

            if (picked[i - 1] + r.com <= w.cap &&
            detour < slack[i] + EPS &&
            part1 + r.len < r.ddl - reach[i - 1] + EPS) {
                if (opt > detour) {
                    opt = detour;
                    delta = opt;
                }
            }
        }
    }

    vector<pair<double, int>> Det;
    for (int i = 0; i < w.S.size(); ++i) {
        pair<double, int> tmp = make_pair(INF, -1);
        if (i == 0) {
            if (w.num + r.com <= w.cap) {
                double detour = swd + sdcache[0] - (reach[0] - w.tim);
                if (detour < slack[i] + EPS) {
                    tmp = make_pair(detour, i);
                }
            }
        } else {
            if (picked[i - 1] + r.com <= w.cap) {
                tmp = Det.back();
                double detour = sdcache[i - 1] + sdcache[i] - (reach[i] - reach[i - 1]);
                if (detour < slack[i] + EPS) {
                    tmp = min(make_pair(detour, i), tmp);
                }
            }
        }
        Det.push_back(tmp);
    }
    for (int i = 0; i < w.S.size(); ++i) {
        if (i < w.S.size() - 1) {
            if (picked[i] > w.cap - r.com) continue;
            double part1 = edcache[i];
            double detour = part1 + edcache[i + 1] - (reach[i + 1] - reach[i]);
            if (Det[i].first + detour < slack[i + 1] + EPS
            && Det[i].first + part1 < r.ddl - reach[i] + EPS) {
                if (opt > Det[i].first + detour) {
                    opt = Det[i].first + detour;
                    delta = opt;
                }
            }
        } else {
            double detour = edcache[i];
            if (Det[i].first + detour < r.ddl - reach[i] + EPS) {
                if (opt > Det[i].first + detour) {
                    opt = Det[i].first + detour;
                    delta = opt;
                }
            }
        }
    }
}


void try_insertion_euclid(Worker& w, Request& r, unordered_map<int, Request>& R, double &delta) {

    double opt = INF;

    if (w.S.empty()) {
        double tmp = w.tim + cost_time_between(w.position, r.s) + r.len;
        if (tmp < r.ddl + EPS && r.com <= w.cap) {
            opt = tmp - w.tim;
            delta = opt;
        }
        return;
    }

    vector<int> &picked = w.picked;
    vector<double> &slack = w.slack;
    vector<time_t> &reach = w.reach;

    for (int i = 0; i <= w.S.size(); ++i) {
        if (i == 0) {
            double part1 = cost_time_between(w.position, r.s);
            pair<double, double> tmp = Pos(w.S[0], R);
            double detour = part1 + r.len + cost_time_between(r.e, tmp) - (reach[0] - w.tim);
            if (w.num + r.com <= w.cap &&
            detour < slack[i] + EPS &&
            part1 + r.len < r.ddl - w.tim + EPS) {
                if (opt > detour) {
                    opt = detour;
                    delta = opt;
                }
            }
        } else if (i == w.S.size()) {
            pair<double, double> tmp = Pos(w.S[i - 1], R);
            double detour = cost_time_between(tmp, r.s) + r.len;
            if (picked[i - 1] + r.com <= w.cap &&
            detour < r.ddl - reach[i - 1] + EPS) {
                if (opt > detour) {
                    opt = detour;
                    delta = opt;
                }
            }
        } else {
            pair<double, double> tmp = Pos(w.S[i - 1], R);
            double part1 = cost_time_between(tmp, r.s);
            tmp = Pos(w.S[i], R);
            double detour = part1 + r.len + cost_time_between(r.e, tmp) - (reach[i] - reach[i - 1]);

            if (picked[i - 1] + r.com <= w.cap &&
            detour < slack[i] + EPS &&
            part1 + r.len < r.ddl - reach[i - 1] + EPS) {
                if (opt > detour) {
                    opt = detour;
                    delta = opt;
                }
            }
        }
    }

    vector<pair<double, int>> Det;
    for (int i = 0; i < w.S.size(); ++i) {
        pair<double, int> tmp = make_pair(INF, -1);
        if (i == 0) {
            if (w.num + r.com <= w.cap) {
                pair<double, double> tmp1 = Pos(w.S[0], R);
                double detour = cost_time_between(w.position, r.s) + cost_time_between(r.s, tmp1) - (reach[0] - w.tim);
                if (detour < slack[i] + EPS) {
                    tmp = make_pair(detour, i);
                }
            }
        } else {
            if (picked[i - 1] + r.com <= w.cap) {
                tmp = Det.back();
                pair<double, double> tmp1 = Pos(w.S[i - 1], R);
                pair<double, double> tmp2 = Pos(w.S[i], R);
                double detour = cost_time_between(tmp1, r.s) + cost_time_between(r.s, tmp2) - (reach[i] - reach[i - 1]);
                if (detour < slack[i] + EPS) {
                    tmp = min(make_pair(detour, i), tmp);
                }
            }
        }
        Det.push_back(tmp);
    }
    for (int i = 0; i < w.S.size(); ++i) {
        if (i < w.S.size() - 1) {
            if (picked[i] > w.cap - r.com) continue;
            pair<double, double> tmp1 = Pos(w.S[i], R);
            pair<double, double> tmp2 = Pos(w.S[i + 1], R);
            double part1 = cost_time_between(tmp1, r.e);
            double detour = part1 + cost_time_between(r.e, tmp2) - (reach[i + 1] - reach[i]);
            if (Det[i].first + detour < slack[i + 1] + EPS
            && Det[i].first + part1 < r.ddl - reach[i] + EPS) {
                if (opt > Det[i].first + detour) {

                    opt = Det[i].first + detour;
                    delta = opt;
                }

            }

        } else {
            pair<double, double> tmp3 = Pos(w.S[i], R);
            double detour = cost_time_between(tmp3, r.e);
            if (Det[i].first + detour < r.ddl - reach[i] + EPS) {
                if (opt > Det[i].first + detour) {
                    opt = Det[i].first + detour;
                    delta = opt;
                }
            }
        }
    }
}


void updateDriverArr(Worker &w, unordered_map<int, Request>& R) {
    double tim = w.tim;
    vector<time_t> &reach = w.reach;

    reach.clear();
    for (int i = 0; i < w.S.size(); ++i) {
        Request &r = R[w.S[i] >> 1];
        if (i == 0) {
            pair<double, double> tmp = Pos(w.S[i], R);
            tim += cost_time_between(w.position, tmp);
        } else {
            pair<double, double> tmp1 = Pos(w.S[i], R);
            pair<double, double> tmp2 = Pos(w.S[i - 1], R);
            tim += cost_time_between(tmp1, tmp2);
        }
        reach.push_back(tim);
    }

    vector<int> &picked = w.picked;
    vector<double> &slack = w.slack;
    picked.clear();
    slack.clear();
    int cc = w.num;

    for (uint i = 0; i < w.S.size(); ++i) {
        if (w.S[i] & 1) {
            cc -= R[w.S[i] >> 1].com;
            slack.push_back(R[w.S[i] >> 1].ddl - reach[i]);
        } else {
            cc += R[w.S[i] >> 1].com;
            slack.push_back(INF);
        }
        picked.push_back(cc);
    }
    for (int i = slack.size() - 2; i >= 0; --i) {
        slack[i] = min(slack[i], slack[i + 1]);
    }
}


void insertion(Worker& w, Request& r, unordered_map<int, Request>& R) {
    double opt = INF;
    vector<int> ret = w.S;
    pair<int, int> ins = make_pair(-1, -1);

    if (w.S.empty()) {
        double kk = cost_time_between(w.position, r.s);
        double tmp = w.tim + kk + r.len;
        if (tmp < r.ddl + EPS && r.com <= w.cap) {
            opt = tmp - w.tim;
            w.S.push_back(r.id << 1);
            w.S.push_back(r.id << 1 | 1);
            updateDriverArr(w, R);
        }
        return;
    }

    // local cache of distances
    double swd = cost_time_between(w.position, r.s);
    vector<double> sdcache(w.S.size(), INF);
    vector<double> edcache(w.S.size(), INF);
    for (int i = 0; i < w.S.size(); ++i) {
        pair<double, double> tmp = Pos(w.S[i], R);
        sdcache[i] = cost_time_between(r.s, tmp);
        edcache[i] = cost_time_between(r.e, tmp);
    }

    vector<int> &picked = w.picked;
    vector<double> &slack = w.slack;
    vector<time_t> &reach = w.reach;

    for (uint i = 0; i <= w.S.size(); ++i) {
        if (i == 0) {
            double part1 = swd;
            double part2 = edcache[i];
            double detour = part1 + r.len + part2 - (reach[0] - w.tim);
            if (w.num + r.com <= w.cap &&
            detour < slack[i] + EPS &&
            part1 + r.len < r.ddl - w.tim + EPS) {
                if (opt > detour) {
                    opt = detour;
                    ins = make_pair(i, i);
                }
            }
        } else if (i == w.S.size()) {
            double part1 = sdcache[i - 1];
            double detour = part1 + r.len;
            if (picked[i - 1] + r.com <= w.cap &&
            detour < r.ddl - reach[i - 1] + EPS) {
                if (opt > detour) {
                    opt = detour;
                    ins = make_pair(i, i);
                }
            }
        } else {
            double part1 = sdcache[i - 1];
            double part2 = edcache[i];
            double detour = part1 + r.len + part2 - (reach[i] - reach[i - 1]);
            if (picked[i - 1] + r.com <= w.cap &&
            detour < slack[i] + EPS &&
            part1 + r.len < r.ddl - reach[i - 1] + EPS) {
                if (opt > detour) {
                    opt = detour;
                    ins = make_pair(i, i);
                }
            }
        }
    }

    vector<pair<double, int>> Det;
    for (int i = 0; i < w.S.size(); ++i) {
        pair<double, int> tmp = make_pair(INF, -1);
        if (i == 0) {
            if (w.num + r.com <= w.cap) {
                double detour = swd + sdcache[i] - (reach[0] - w.tim);
                if (detour < slack[i] + EPS) {
                    tmp = make_pair(detour, i);
                }
            }
        } else {
            if (picked[i - 1] + r.com <= w.cap) {
                tmp = Det.back();
                double detour = sdcache[i - 1] + sdcache[i] - (reach[i] - reach[i - 1]);
                if (detour < slack[i] + EPS) {
                    tmp = min(make_pair(detour, i), tmp);
                }
            }
        }
        Det.push_back(tmp);
    }
    for (uint i = 0; i < w.S.size(); ++i) {
        if (i < w.S.size() - 1) {
            if (picked[i] > w.cap - r.com) continue;
            double part1 = edcache[i];
            double detour = part1 + edcache[i + 1] - (reach[i + 1] - reach[i]);
            if (Det[i].first + detour < slack[i + 1] + EPS
            && Det[i].first + part1 < r.ddl - reach[i] + EPS) {
                if (opt > Det[i].first + detour) {
                    opt = Det[i].first + detour;
                    ins = make_pair(Det[i].second, i + 1);
                }
            }
        } else {
            double detour = edcache[i];
            if (Det[i].first + detour < r.ddl - reach[i] + EPS) {
                if (opt > Det[i].first + detour) {
                    opt = Det[i].first + detour;
                    ins = make_pair(Det[i].second, i + 1);
                }
            }
        }
    }
    if (ins.first > -1) {
        ret.clear();
        for (int j = 0; j < ins.first; ++j) {
            ret.push_back(w.S[j]);
        }
        ret.push_back(r.id << 1);
        for (int j = ins.first; j < ins.second; ++j) {
            ret.push_back(w.S[j]);
        }
        ret.push_back(r.id << 1 | 1);
        for (int j = ins.second; j < w.S.size(); ++j) {
            ret.push_back(w.S[j]);
        }

        w.S = ret;
        updateDriverArr(w, R);
    }
}


bool routing(ProblemInstance* problem, Group& group, time_t now_time, double delay_pick, double& min_slack_time, double& cost_time) {
    // adapter for insertion
    Worker w;
    unordered_map<int, Request> R;
    for (int id : group.get_member_ids()) {
        Order& order_ptr = problem->get_single_order(id);
        Request tmp;
        tmp.s = order_ptr.get_pick_location();
        tmp.e = order_ptr.get_drop_location();
        tmp.tim = order_ptr.get_pick_time();
        tmp.r_tim = now_time - tmp.tim;  // attention here
        tmp.ddl = order_ptr.get_ddl();
        tmp.len = order_ptr.get_cost_time();
        tmp.com = order_ptr.get_passenger_count();
        tmp.id = order_ptr.get_id();
        R.insert({order_ptr.get_id(), tmp});
        w.position = tmp.s;
    }
    w.tim = now_time + delay_pick;
    w.cap = CAPACITY_CONSTRAINT;
    w.num = 0;

    for (auto& kv : R) {
        double fit = INF;
        try_insertion(w, kv.second, R, fit);
        if (fit >= INF)
            return false;
        insertion(w, kv.second, R);
    }
    w.position = R[w.S[0] >> 1].s;
    updateDriverArr(w, R);

    min_slack_time = INF;
    for (double slack_time : w.slack) {
        if (slack_time < min_slack_time) {
            min_slack_time = slack_time;
        }
    }
    group.set_min_slack_time(min_slack_time);

    group.clear_response_time();
    group.clear_detour_time();

    for (int i = 0; i < w.S.size(); i++) {
        Request &r = R[w.S[i] >> 1];
        if (i == 0) {
            group.set_start_position(r.s);
            int gid = -1;
            in_which_grid_id(r.s.first, r.s.second, gid);
            group.set_start_grid_id(gid);
        }
        if (i == w.S.size() - 1) {
            group.set_end_position(r.e);
            int gid = -1;
            in_which_grid_id(r.e.first, r.e.second, gid);
            group.set_end_grid_id(gid);
            cost_time = (w.reach[i] - w.reach[0]);
            if (cost_time > group.get_order_total_cost(problem))
                return false;
            group.set_driver_cost(cost_time);
        }


        if (w.S[i] & 1) {
            double order_finish_cost = (w.reach[i] - R[w.S[i] >> 1].tim);
            double response_time = (now_time - R[w.S[i] >> 1].tim);
            double detour_time = order_finish_cost - response_time - R[w.S[i] >> 1].len;
            group.add_response_time(response_time);
            group.add_detour_time(detour_time);
            group.response_time[w.S[i] >> 1] = response_time;
            group.detour_time[w.S[i] >> 1] = detour_time;
            group.extra_time[w.S[i] >> 1] = response_time + detour_time;
        }
    }

    return true;
}

bool is_accept(ProblemInstance* problem, Driver& driver, Group& group, time_t now_time,
                       double& wait_delay_time, double& driver_to_pick_time,
                       double& cost_time) {
    if (group.get_capacity() > driver.get_capacity())
        return false;

    double location_x, location_y;
    std::tie(location_x, location_y) = driver.get_location();

    bool ret;
    double pick_distance =
            abs(location_x - group.get_start_position().first) +
            abs(location_y - group.get_start_position().second);

    driver_to_pick_time = pick_distance / group.get_speed();

    wait_delay_time = max(int(driver.end_time_list.back() - now_time), 0);
    if (driver.end_time_list.back() == 0)
        wait_delay_time = 0;

    if (wait_delay_time + driver_to_pick_time > group.get_min_slack_time())
        return false;
    else{
        double min_slack_time = INF, min_cost_time = INF;
        if (routing(problem, group, now_time, wait_delay_time + driver_to_pick_time, min_slack_time, min_cost_time)) {
            cost_time = group.get_driver_cost();
            return true;
        }
        return false;
    }
}

bool serve(ProblemInstance* problem, Driver& driver, Group& group, time_t now_time, time_t pick_time) {
    // pick
    int gid = -1;
    double location_x, location_y;
    std::tie(location_x, location_y) = driver.get_location();

    driver.start_time_list.push_back(now_time);
    driver.end_time_list.push_back(now_time + pick_time);

    in_which_grid_id(location_x, location_y, gid);
    driver.history_gid.push_back(gid);
    driver.history_payment.push_back(0);
    driver.history_driver_cost.push_back(0);
    driver.history_pick_cost.push_back(0);
    driver.history_num.push_back(0);

    driver.add_num_serve(group.get_capacity(), 1);
    // serve
    driver.start_time_list.push_back(now_time + pick_time);
    driver.end_time_list.push_back(now_time + (time_t)group.get_driver_cost());

    driver.history_gid.push_back(group.get_end_grid_id());
    driver.history_payment.push_back(group.get_order_total_cost(problem));
    driver.history_driver_cost.push_back(group.get_driver_cost());
    driver.history_pick_cost.push_back(group.get_pick_cost());
    driver.history_num.push_back(group.get_capacity());

    driver.set_location(group.get_end_position().first, group.get_end_position().second);

    return true;
}