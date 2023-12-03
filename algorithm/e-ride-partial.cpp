#include "e-ride-partial.h"
#include "../util/gaussian/gaussian.h"

GMM* gmm;

void init_variables() {
    if (USE_DISTRIBUTION_PREDICT) {
        dqn = new DQNAgent(city, MODEL_TRAIN_ALGO, DEADLINE_GAMMA, USE_TARGET_VALUE, MODE, TRAIN_ROUND, GRID_SIZE);
    }
    if (algo_name == "algo-gaussian") {
        predict_by_ground_truth(expect_min_time, expect_round);
        gmm = new GMM();
        gmm->Init(sg->extra_time_sample_set, 5);
        gmm->train();
        sg = nullptr;
        problem = nullptr;
    }

    problem = new ProblemInstance();
    sg = new ShareabilityGraph();
    problem->init_workers();
    gi = new GridIndex();
    gi->add_drivers(problem->get_all_drivers());
    sg->init_driver_distribution(problem->get_all_drivers());
    sg->now_time = problem->get_start_time();
    sg->init_timestamp = problem->get_start_time();
    batch_time = problem->get_start_time();
    tmp.first = -1;
}


bool predict_process(unordered_set<int>& working_set) {
    vector<State> states;
    vector<int> ids;
    for (int id : working_set) {
        State s = sg->get_current_state(id);
        states.push_back(s);
        ids.push_back(id);
    }

    vector<double> results(working_set.size());
    torch::Tensor result = dqn->estimate_batch(states, true, false);
    std::vector<float> vec(result.data_ptr<float>(), result.data_ptr<float>() + result.numel());

    for (int i; i < ids.size(); i++) {
        expect_min_time[ids[i]] = vec[i];
    }

    return true;
}


pair<double, double> get_best_value(int exist_order_id) {
    double tr = 0, td = 0;
    pair<int, vector<double>> tmp1 = make_pair(-1, vector<double>(7));
    try {
        sg->get_best_partner(problem->get_single_order(exist_order_id), tmp1);
        tr = tmp1.second[3];
        td = tmp1.second[4];
    } catch (exception& e) {
        tr = 0;
        td = 0;
    }
    return {tr, td};
}


bool make_decision_rewrite(int exist_order_id, bool& is_policy) {
    is_policy = false;
    double target_value;

    if (USE_DISTRIBUTION_PREDICT)
        sg->add_experience(exist_order_id, nullptr, false);

    if (USE_DISTRIBUTION_PREDICT or algo_name == "algo-gaussian") {
        double now_best_value = INF;
        Group best_group;
        if (sg->get_best_group(problem->get_single_order(exist_order_id), best_group)) {
            double min_slack_time = INF, min_cost_time = INF;
            now_best_value = best_group.get_avg_extra_waiting_time();
            vector<double> tmp_extra;
            best_group.get_extra_time(tmp_extra);
            for (auto extra : tmp_extra)
                sg->extra_time_sample_set.emplace_back(extra);
        } else {
            return false;
        }

        // 最好的group过期
        if (best_group.get_expire_timeslot() <= sg->transform_to_timeslot(sg->now_time)) {
            sg->update_best_group(problem->get_single_order(exist_order_id));
            return false;
        }

        double expect_value = 0;
        for (auto& id : best_group.get_member_ids()) {
            target_value = problem->get_single_order(id).get_original_slack() / 3;
            if (algo_name == "algo-gaussian") {
                target_value = find_maximum_x(gmm->means, gmm->sigmas, gmm->probilities, problem->get_single_order(id).get_original_slack());
            }

            expect_min_time[id] = problem->get_single_order(id).get_original_slack() - expect_min_time[id];
            if ((MODE == 5 and count_all < 10000) or algo_name == "algo-gaussian") {
                expect_min_time[id] = target_value;
            }
            expect_value += expect_min_time[id];
            best_group.expected_time[id] = expect_min_time[id];
        }
        expect_value /= best_group.get_member_ids().size();

        expect_value += 20;

        if (TRAIN_UNDER_EXPECT) {
            if (now_best_value < expect_value) {
                is_policy = true;
                return true;
            }
            else
                return false;
        }
        else {
            int count_dispatch = 0, count_delay = 0;
            for (int id : best_group.get_member_ids()) {
                double value_dispatch = problem->get_single_order(id).get_original_slack() - best_group.detour_time[id] - best_group.response_time[id];
                State s = sg->get_current_state(id);
                s.timeAndWait[1] += 1;
                double value_delay = dqn->estimate(s);

                float x = float(int(rand()) % 10000) / 10000;
                bool other_action_prob = x < 0.005;

                if (value_dispatch > value_delay or other_action_prob)
                    count_dispatch += 1;
                else
                    count_delay += 1;
            }
            if (count_dispatch > count_delay)
                return true;
            else
                return false;
        }

    } else {

        float x = float(int(rand()) % 10000) / 10000;
        if (x < CHECK_TRUE_POSSIBILITY)
            return true;
        else
            return false;
    }
}


bool dispatch_to_be_served_group() {
    bool is_delay;
    auto iter = sg->get_to_be_served_groups().begin();

    while (iter != sg->get_to_be_served_groups().end()) {
        if (sg->try_dispatch_group(*iter, is_delay)) {
            sg->get_served_groups().push_back(*iter);
            sg->get_to_be_served_groups().erase(iter);

        } else
            iter++;
    }

    return true;
}


void show_result_by_group_list() {
    int count_all = 0, count_served = 0, count_failed_single = 0, count_failed_delayed = 0;
    double total_cost = 0, total_cost_served = 0, total_cost_penalty = 0;
    double total_revenue = 0, total_revenue_pay_request = 0, total_revenue_pay_driver = 0;
    double total_response_time = 0, total_detour_time = 0;
    double oo = 0, oo_penalty = 0;
    double rate_served, rate_unserved;
    double total_pick_cost;
    vector<int> how_dispatch(8);
    vector<int> count_gid_on_policy(NUM_GRID_X * NUM_GRID_Y), count_gid_off_policy(NUM_GRID_X * NUM_GRID_Y);
    unordered_map<int, int> capacity_count;
    for (auto& group : sg->get_served_groups()) {
        if (capacity_count.count(group.get_capacity()) <= 0)
            capacity_count[group.get_capacity()] = 0;
        capacity_count[group.get_capacity()]++;
        count_all += group.get_capacity();
        count_served += group.get_capacity();
        total_response_time += group.get_total_extra_response_time();
        total_detour_time += group.get_total_extra_detour_time();

        total_cost_served += ALPHA * group.get_driver_cost();
        total_pick_cost += group.get_pick_cost();
        total_revenue_pay_request += CR * group.get_order_total_cost(problem);
        total_revenue_pay_driver += CW * group.get_driver_cost();
        how_dispatch[group.get_how_dispatch()] += group.get_capacity();
        for (auto& id : group.get_member_ids()) {
            int gid = -1;
            Order& order = problem->get_single_order(id);
            in_which_grid_id(order.get_pick_location().first, order.get_pick_location().second, gid);
            double residence = expect_min_time[id] - group.get_avg_extra_waiting_time() + group.get_pick_cost();

            if (group.get_how_dispatch() == POLICY) {
                on_policy_residence[gid] += residence;
                count_gid_on_policy[gid] += 1;
            } else {
                off_policy_residence[gid] += residence;
                count_gid_off_policy[gid] += 1;
            }
        }
    }
    oo = (total_response_time + total_detour_time);
    for (auto& group : sg->get_to_be_served_groups()) {
        count_all += group.get_capacity();
        count_failed_delayed += group.get_capacity();
        total_cost_penalty += (1 * PENALTY * group.get_order_total_cost(problem));
        oo_penalty += group.get_order_total_slack(problem);
    }
    for (auto& group : sg->get_failed_groups()) {
        count_all += group.get_capacity();
        count_failed_single += group.get_capacity();
        total_cost_penalty += (1 * PENALTY * group.get_order_total_cost(problem));
        oo_penalty += group.get_order_total_slack(problem);
        how_dispatch[group.get_how_dispatch()] += group.get_capacity();
    }
    oo += oo_penalty;
    rate_served = count_served * 1.0 / count_all;
    rate_unserved = (count_failed_delayed + count_failed_single) * 1.0 / count_all;


    printf("%s\n", algo_name.c_str());
    printf("request size: %d\n", count_all);
    printf("served rate: %.4f\n", rate_served);
    printf("unserved rate: %.4f\n", rate_unserved);

    printf("cost(s+p): %.0lf\n", (total_cost_served + total_cost_penalty) / (COMPARE_SPEED / UNI_SPEED));
    printf("cost(s): %.0lf\n", total_cost_served / (COMPARE_SPEED / UNI_SPEED));
    printf("cost(p): %.0lf\n", total_cost_penalty / (COMPARE_SPEED / UNI_SPEED));

    printf("revenue(r-w): %.0lf\n", (total_revenue_pay_request - total_revenue_pay_driver) / (COMPARE_SPEED / UNI_SPEED));
    printf("revenue(r): %.0lf\n", total_revenue_pay_request / (COMPARE_SPEED / UNI_SPEED));
    printf("revenue(w): %.0lf\n", total_revenue_pay_driver / (COMPARE_SPEED / UNI_SPEED));

    printf("avg_extra(r+d): %.4lf\n", total_response_time / count_served + (total_detour_time / count_served) / (COMPARE_SPEED / UNI_SPEED));
    printf("avg_extra(r): %.4lf\n", (total_response_time / count_served));
    printf("avg_extra(d): %.4lf\n", (total_detour_time / count_served) / (COMPARE_SPEED / UNI_SPEED));

    cout << "running time (ms): " << (algorithm_finish_time - algorithm_start_time) / 1000 << endl;
    cout << "total_max_routing_cost_time (ms): " << sg->total_max_routing_cost_time / 1000 << endl;
    cout << "parallel_save_time (ms): " << sg->parallel_save_time / 1000 << endl;

}
