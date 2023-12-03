#include "e-ride-partial.h"


void torch_cuda_cudnn_test() {
    assert(torch::cuda::is_available());
    assert(torch::cuda::cudnn_is_available());

    torch::Tensor foo = torch::arange(25).reshape({5, 5});
    torch::Tensor bar = torch::einsum("ii", foo);

    assert(bar.item<int>() == 60);
}

void main_loop_rewrite() {
    algorithm_start_time = nowTime();

    init_variables();

    FRAGMENT = TIMESLOT_DELTA;

    int count_accept = 0, count_delay = 0;

    while (batch_time < problem->get_end_time()) {
        vector<Order> batch_order = problem->batch(batch_time);
        batch_time += FRAGMENT;
        batch_round++;
        is_new_timeslot = true;

        for (const auto& order : batch_order) {
            if (count_all >= REQUEST_SIZE)
                break;
            if (count_all != 0 and count_all % 100 == 0){
                if (TRAIN)
                    dqn->train();
            }


            sg->now_time = order.get_pick_time();
            sg->total_len += order.get_cost_time();
            count_all++;

            int now_time_slot = sg->transform_to_timeslot(sg->now_time);
            unordered_map<int, unordered_set<int>>& expired_edges = sg->get_expired_edges(now_time_slot);
            for (auto& kv : expired_edges) {
                int u = kv.first;
                for (auto& v : kv.second) {
                    Group group(problem, {u, v});
                    sg->delete_node_rewrite(group, false);
                }

            }

            if (sg->add_node_rewrite(order)) {
            }

            if (is_new_timeslot or REQUEST_CHECK) {
                unordered_set<int> working_set(sg->get_exist_orders());
                if (algo_name == "algo-expect") predict_process(working_set);

                for (int exist_order_id : working_set) {
                    if (sg->get_exist_orders().count(exist_order_id) == 0) continue;
                    Order exist_order = problem->get_single_order(exist_order_id);
                    if (sg->now_time > exist_order.get_durable() + exist_order.get_pick_time()) {
                        sg->trigger_event_rewrite(exist_order, TIMEOUT);
                    }
                    else if (make_decision_rewrite(exist_order_id, is_policy)) {
                        if (is_policy)
                            sg->trigger_event_rewrite(exist_order, POLICY);
                        else
                            sg->trigger_event_rewrite(exist_order, CHECK);
                        count_accept ++;
                    } else {
                        count_delay ++;
                    }
                }
                is_new_timeslot = false;
            }

        }
        dispatch_to_be_served_group();
        if (count_all >= REQUEST_SIZE)
            break;
    }

    // clear all orders
    unordered_set<int> working_set = sg->get_exist_orders();
    for (int exist_order_id : working_set) {
        if (sg->get_exist_orders().count(exist_order_id) == 0) continue;

        Order exist_order = problem->get_single_order(exist_order_id);
        sg->trigger_event_rewrite(exist_order, CLEAR);
    }

    algorithm_finish_time = nowTime();

    show_result_by_group_list();
}

int main(int argc, char **args) {
    torch_cuda_cudnn_test();
    city = "test";
    DEADLINE_GAMMA = 1.6;
    WORKER_NUM = 5000;
    TIMESLOT_DELTA = 10;
    CAPACITY_CONSTRAINT = 3;
    REQUEST_SIZE = 10000;
    algo_name = "algo-expect";
    MODEL_TRAIN_ALGO = "algo-expect";
    MODE = 5;
    TRAIN_ROUND = 1;
    TRAIN = false;

    if (argc > 8) city = string(args[1]);
    if (argc > 8) algo_name = string(args[2]);
    if (argc > 8) REQUEST_SIZE = stoi(args[3]);
    if (argc > 8) WORKER_NUM = stoi(args[4]);
    if (argc > 8) DEADLINE_GAMMA = stod(args[5]);
    if (argc > 8) CAPACITY_CONSTRAINT = stoi(args[6]);
    if (argc > 8) TIMESLOT_DELTA = stoi(args[7]);
    if (argc > 8) {
        string desFile = string(args[8]);
        freopen(desFile.c_str(), "w", stdout);
    }
    if (argc > 10) {
        MODE = stoi(args[9]);
        TRAIN_ROUND = stoi(args[10]);
        TRAIN = true;
    } else if (argc > 8)
        TRAIN = false;

    if (argc > 11) {
        GRID_SIZE = stoi(args[11]);
        NUM_GRID_X = GRID_SIZE;
        NUM_GRID_Y = GRID_SIZE;
        TRAIN = false;
    }
    if (argc > 12) {
        int train_flag = stoi(args[12]);
        if (train_flag == 0)
            TRAIN = false;
        else
            TRAIN = true;
    }

    switch (MODE) {
        case 1:
        USE_TARGET_VALUE = true;
        TRAIN_UNDER_EXPECT = false;
        break;
        case 2:
        USE_TARGET_VALUE = true;
        TRAIN_UNDER_EXPECT = true;
        break;
        case 3:
        USE_TARGET_VALUE = false;
        TRAIN_UNDER_EXPECT = false;
        break;
        case 4:
        USE_TARGET_VALUE = false;
        TRAIN_UNDER_EXPECT = true;
        break;
        case 5:
        USE_TARGET_VALUE = true;
        TRAIN_UNDER_EXPECT = true;
        break;
        default:
            USE_TARGET_VALUE = true;
            TRAIN_UNDER_EXPECT = true;
            break;
    }

    COMPARE_SPEED = 0.00009;

    if (algo_name == "algo-expect") {
        USE_DISTRIBUTION_PREDICT = true;
        DURABLE_BASE = 0.8;
        REQUEST_CHECK = false;
    }
    else if (algo_name == "algo-gaussian") {
        USE_DISTRIBUTION_PREDICT = false;
        DURABLE_BASE = 0.8;
        REQUEST_CHECK = false;
        TRAIN = false;
        TRAIN_UNDER_EXPECT = true;
    }
    else {
        USE_DISTRIBUTION_PREDICT = false;
        TRAIN = false;
        if (algo_name == "algo-online") {
            DURABLE_BASE = 0.8;
            CHECK_TRUE_POSSIBILITY = 1;
            REQUEST_CHECK = true;
        }
        else if (algo_name == "algo-timeout") {
            DURABLE_BASE = 0.8;
            CHECK_TRUE_POSSIBILITY = 0;
            REQUEST_CHECK = true;
        }
        else if (algo_name == "algo-check")
            CHECK_TRUE_POSSIBILITY = 0.001;
    }
    main_loop_rewrite();
    return 0;
}
