#include "value_distribution.h"



void predict_by_ground_truth(unordered_map<int, double>& expect_min_time, unordered_map<int, int>& expect_round) {
    const string EXPECT_MIN_TIME_PATH = "../file/expect_min_time" + to_string(int(DEADLINE_GAMMA)) + ".txt";
    const string EXPECT_ROUND_PATH = "../file/expect_round" + to_string(int(DEADLINE_GAMMA)) + ".txt";

    const string RESULT_DIC = "../file/result";

    if (load_map(expect_min_time, EXPECT_MIN_TIME_PATH) and
            load_map<int>(expect_round, EXPECT_ROUND_PATH))
        return ;

    problem = new ProblemInstance();
    sg = new ShareabilityGraph();
    sg->now_time = problem->get_start_time();

    vector<vector<int>> order_distribution;
    for (time_t batch_time = problem->get_start_time(); batch_time < problem->get_end_time(); batch_time += FRAGMENT) {
        // 这个for循环用于把所有订单分割成若干个batch方便查询
        order_distribution.emplace_back();
        vector<Order> batch_order = problem->batch(batch_time);
        for (const auto& order : batch_order)
            order_distribution.back().emplace_back(order.get_id());
    }

    problem->reset_batch_iter();

    int round = 0;
    for (time_t batch_time = problem->get_start_time(); batch_time < problem->get_end_time(); batch_time += FRAGMENT) {
        vector<Order> batch_order = problem->batch(batch_time);
        for (const auto& order : batch_order) {
            sg->now_time = order.get_pick_time();
            int tmp_time = INF;
            bool flag = true;

            for (int i = 0; i < WATCHING_WINDOW_SIZE; i++) {
                int now_watching_round = round + i;
                if (now_watching_round >= order_distribution.size()) break;

                flag = false;
                vector<int>& order_series = order_distribution[now_watching_round];
                double avg_time = 0;
                int count = 0;
                for (const auto& tmp_id : order_series) {
                    if (tmp_id == order.get_id())
                        continue;
                    Order tmp_order = problem->get_single_order(tmp_id);
                    sg->now_time = tmp_order.get_pick_time();

                    vector<double> o1_attr(3);
                    vector<double> o2_attr(3);
                    vector<double> all_attr(1);
                    if (is_matchable(order, tmp_order, sg->now_time, o1_attr, o2_attr, all_attr)) {
                        count++;
                        avg_time += (max(int(sg->now_time - order.get_pick_time()), 0) + o1_attr[2]);
                        sg->extra_time_sample_set.push_back((max(int(sg->now_time - order.get_pick_time()), 0) + o1_attr[2]));
                    }
                }
                if (count == 0) continue;
                avg_time /= count;

                if (avg_time < tmp_time) {
                    tmp_time = avg_time;
                    expect_min_time[order.get_id()] = avg_time;
                    expect_round[order.get_id()] = now_watching_round;
                }
            }

            if (flag) {
                expect_min_time[order.get_id()] = INF;
                expect_round[order.get_id()] = round;
            }

        }
        round++;
    }

}
