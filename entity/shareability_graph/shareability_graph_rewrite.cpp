#ifndef TEST_SHAREABILITY_GRAPH_REWRITE_H
#define TEST_SHAREABILITY_GRAPH_REWRITE_H

#include "shareability_graph_rewrite.h"


ShareabilityGraph::ShareabilityGraph() {
    heatmap = vector<vector<int>>(NUM_GRID_Y, vector<int> (NUM_GRID_X));
    is_sorted_max = (sorted_attr_index != 2);
    driver_distribution.resize(NUM_GRID_Y * NUM_GRID_X);
    start_distribution.resize(NUM_GRID_Y * NUM_GRID_X);
    end_distribution.resize(NUM_GRID_Y * NUM_GRID_X);
    unsigned seed = time(0);
    srand(seed);
}


bool ShareabilityGraph::trigger_event_rewrite(const Order& order, RiderEvent event) {
    bool flag, is_timeout = false;
    if (event == TIMEOUT or event == CLEAR)
        is_timeout = true;
    flag = dispatch_rewrite(order, is_timeout, event);
    return flag;
}


bool ShareabilityGraph::get_max_neighbor(const Order& order, int& max_id, vector<double>& revenue_attr) {
    unordered_map<int, vector<double>> neighbors = graph[order.get_id()];

    double max_attr;
    if (neighbors.empty()) {
        // single 的话返回当前订单的等待时间
        if (sorted_attr_index == 2)
            max_attr = (double)(now_time - order.get_pick_time());
        return false;
    }

    bool ret = false;
    max_attr = (is_sorted_max) ? -1 * INF : INF;

    vector<double> o1_attr(3), o2_attr(3), total_attr_vec(3);
    double total_attr = 0;
    for (const auto& neighbor : neighbors) {
        if (not compute_group_attr(order, problem->get_single_order(neighbor.first), now_time,
                                   total_attr_vec, o1_attr, o2_attr))
            // 邻居应该都是可以share的才对，除非now_time导致的过期
            continue;
        total_attr = total_attr_vec[sorted_attr_index];

        if ((is_sorted_max and total_attr >= max_attr) or ((not is_sorted_max) and total_attr <= max_attr)) {
            revenue_attr.swap(total_attr_vec);
            max_attr = total_attr;
            max_id = neighbor.first;
            ret = true;
        }
    }

    if (ret) {
        best_partner_map[order.get_id()] = {max_id, revenue_attr};
        be_best_partner_map[max_id].insert(order.get_id());
    }


    return ret;
}



bool ShareabilityGraph::delete_node(const Order& order) {
    gi->remove_from_grid(order);

    for (auto iter = graph[order.get_id()].begin(); iter != graph[order.get_id()].end(); iter++)
        graph[iter->first].erase(order.get_id());

    for (const auto& iter : be_best_partner_map[order.get_id()]) {
        if (working_set.count(iter) == 0)
            continue;
        int max_id;
        vector<double> max_attr(3);
        if (get_max_neighbor(problem->get_single_order(iter), max_id, max_attr)) {
            best_partner_map[iter] = make_pair(max_id, max_attr);
            be_best_partner_map[max_id].insert(iter);
        }
        else {
            double tmp = (is_sorted_max) ? -1 * INF : INF;
            vector<double> tmp_attr_vec = {tmp, tmp, tmp};
            best_partner_map[iter] = make_pair(-1, tmp_attr_vec);
        }
    }


    //delete node
    graph.erase(order.get_id());
    working_set.erase(order.get_id());
    best_partner_map.erase(order.get_id());
    be_best_partner_map.erase(order.get_id());
    return true;
}


bool ShareabilityGraph::add_node(const Order& order) {
    auto *p = new unordered_map<int, vector<double>>();
    graph[order.get_id()] = *p;

    bool match_flag = false;
    vector<double> total_attr_vec(7);
    double total_attr = 0;
    double best_partner_attr, best_order_attr;
    int best_partner_id, best_order_id;

    // insert into grid index
    gi->insert_into_grid(order);

    // insert order entry in best_partner_map
    double max_attr = (is_sorted_max) ? -1 * INF : INF;
    vector<double> tmp_attr_vec = {max_attr, max_attr, max_attr, max_attr, max_attr, max_attr, max_attr};
    best_partner_map[order.get_id()] = make_pair(-1, tmp_attr_vec);
    for (int candidate_order_id: working_set) {
        // todo：这里要改成get_max的版本
        Order candidate_order = problem->get_single_order(candidate_order_id);

        vector<double> o1_attr(3), o2_attr(3), all_attr(4);
        if (is_matchable(order, candidate_order, now_time, o1_attr, o2_attr, all_attr)) {
            // insert into graph
            graph[order.get_id()].insert({candidate_order_id, o1_attr});
            graph[candidate_order_id].insert({order.get_id(), o2_attr});

            match_flag = true;
        }
    }
    unordered_map<int, vector<double>> neighbors = graph[order.get_id()];
    for (const auto & neighbor : neighbors) {
        update_best_partner_attr_now(problem->get_single_order(neighbor.first));
    }
    update_best_partner_attr_now(order);
    working_set.insert(order.get_id());
    return match_flag;
}


bool ShareabilityGraph::insert_edge(int o1_id, int o2_id, time_t expire_timestamp) {
    int expire_timeslot = transform_to_timeslot(expire_timestamp);
    rewrite_graph[o1_id].insert({o2_id, expire_timeslot});
    rewrite_graph[o2_id].insert({o1_id, expire_timeslot});


    edge_expire_index[expire_timeslot][o1_id].insert(o2_id);
}


bool ShareabilityGraph::has_edge(int o1_id, int o2_id) {
    return (rewrite_graph[o1_id].count(o2_id) > 0) and (rewrite_graph[o2_id].count(o1_id) > 0);
}


bool ShareabilityGraph::add_node_rewrite(const Order &order) {
    rewrite_graph.insert({order.get_id(), *(new unordered_map<int, int>())});

    bool has_neighbor = false;

    // insert into grid index
    gi->insert_into_grid(order);

    Group* tmp_group = new Group(problem, {order.get_id()});
    update_order_distribution(tmp_group, true);
//    delete(tmp_group);

    // (1) insert order entry in graph
    for (int candidate_order_id: working_set) {
        Order candidate_order = problem->get_single_order(candidate_order_id);

        double min_slack_time = INF, min_cost_time = INF;
        Group tmp_group(problem, {order.get_id(), candidate_order_id});
        if (routing(problem, tmp_group, now_time, 0, min_slack_time, min_cost_time)) {
            insert_edge(order.get_id(), candidate_order_id, now_time + min_slack_time);
            has_neighbor = true;
            count_neighbor_arrive += 1;
        }
    }

    // (2) enumerate and update best_group_map, be_best_group_map
    update_best_group(order);
    working_set.insert(order.get_id());
    return has_neighbor;
}


bool ShareabilityGraph::delete_node_rewrite(Group& group, bool is_departure) {
    unordered_set<int> original_orders_id;
    unordered_set<int> candidate_orders_id;
    Group group_copy(group);
    if (is_departure)
        update_order_distribution(&group_copy, false);
    if (is_departure) {
        for (int order_id : group_copy.get_member_ids()) {
            Order& order_ptr = problem->get_single_order(order_id);
            // remove from grid index
            gi->remove_from_grid(order_ptr);

            // delete edge
            for (auto& kv : rewrite_graph[order_ptr.get_id()]) {
                rewrite_graph[kv.first].erase(order_ptr.get_id());
                count_neighbor_leave += 1;
            }

            // add be_best_group update id
            for (auto& id : be_best_group_map[order_ptr.get_id()])
                original_orders_id.insert(id);

            rewrite_graph[order_ptr.get_id()].clear();
            best_group_map[order_ptr.get_id()] = Group();
            be_best_group_map[order_ptr.get_id()].clear();

            rewrite_graph.erase(order_ptr.get_id());
            working_set.erase(order_ptr.get_id());
            best_group_map.erase(order_ptr.get_id());
            be_best_group_map.erase(order_ptr.get_id());
        }
        set_difference(original_orders_id.begin(), original_orders_id.end(),
                       group_copy.get_member_ids().begin(), group_copy.get_member_ids().end(),
                       inserter(candidate_orders_id, candidate_orders_id.begin()));

    } else {
        unordered_set<int> members = group_copy.get_member_ids();
        auto iter = members.begin();
        Order& o1 = problem->get_single_order(*iter);
        iter++;
        Order& o2 = problem->get_single_order(*iter);

        rewrite_graph[o1.get_id()].erase(o2.get_id());
        rewrite_graph[o2.get_id()].erase(o1.get_id());

        set_intersection(
                be_best_group_map[o1.get_id()].begin(), be_best_group_map[o1.get_id()].end(),
                be_best_group_map[o2.get_id()].begin(), be_best_group_map[o2.get_id()].end(),
                inserter(candidate_orders_id, candidate_orders_id.begin()));
    }

    for (auto& id : candidate_orders_id) {
        update_best_group(problem->get_single_order(id));
    }
}


bool ShareabilityGraph::update_best_group(const Order& order) {
    vector<Group> groups;
    groups.reserve(26);
    enumerate_group(order, groups);
    Group *best_group_ptr = nullptr;
    bool ret = false;

    double best_group_value = INF, best_group_cost = INF;
    time_t start_parallel_time = nowTime();
    time_t max_routing_cost_time = 0;
    for (auto& group : groups) {
        time_t start_routing_time = nowTime();
        double min_slack_time = INF, min_cost_time = INF;
        bool shareable = false;
        shareable = routing(problem, group, now_time, 0, min_slack_time, min_cost_time);
        if (not shareable)
            continue;
        group.set_expire_timeslot(transform_to_timeslot(now_time + min_slack_time));
        double current_group_value = group.get_driver_cost() / group.get_capacity();
        if (current_group_value < best_group_value) {
            if (group.get_min_slack_time() < 50)
                continue;
            best_group_value = current_group_value;
            best_group_cost = group.get_driver_cost() / group.get_capacity();
            best_group_ptr = &group;
            ret = true;
        }
        time_t finish_routing_time = nowTime();
        time_t routing_cost_time = finish_routing_time - start_routing_time;
        if (max_routing_cost_time < routing_cost_time)
            max_routing_cost_time = routing_cost_time;
    }
    time_t end_parallel_time = nowTime();
    parallel_save_time += (end_parallel_time - start_parallel_time - max_routing_cost_time);
    total_max_routing_cost_time += max_routing_cost_time;


    if (not ret) {
        best_group_map.erase(order.get_id());
        return ret;
    }

    double min_slack_time = INF, min_cost_time = INF;
    routing(problem, *best_group_ptr, now_time, 0, min_slack_time, min_cost_time);

    for (auto& r_id : best_group_ptr->get_member_ids()) {
        double current_best_value = INF;
        if (best_group_map.count(r_id) > 0) {
            current_best_value = best_group_map[r_id].get_avg_extra_waiting_time();
        }

        if (best_group_value < current_best_value or r_id == order.get_id()) {
            for (auto& r_jd : best_group_map[r_id].get_member_ids())
                be_best_group_map[r_jd].erase(r_id);

            for (auto& r_jd : best_group_ptr->get_member_ids()) {
                if (r_id != r_jd)
                    be_best_group_map[r_jd].insert(r_id);
            }
            best_group_map[r_id] = *best_group_ptr;
        }
    }

    return ret;
}

bool ShareabilityGraph::dispatch_rewrite(const Order& order, bool is_timeout, RiderEvent event) {
    bool is_delay = true;
    bool has_feasible_driver = false;
    bool has_feasible_group = false;
    bool is_single_group = false;
    bool is_now_updated = false;

    Group* best_group;
    Group* single_group_ptr;
    double min_slack_time = INF, min_cost_time = INF;

    if (best_group_map.count(order.get_id()) <= 0) {
        has_feasible_group = update_best_group(order);
        is_now_updated = true;
    }

    if (best_group_map.count(order.get_id()) > 0) {
        best_group = &best_group_map[order.get_id()];
        has_feasible_group = routing(problem, *best_group, now_time, 0, min_slack_time, min_cost_time);
        if (has_feasible_group)
            best_group = &best_group_map[order.get_id()];

        if (not has_feasible_group and not is_now_updated and update_best_group(order)) {
            min_slack_time = INF;
            best_group = &best_group_map[order.get_id()];
            has_feasible_group = true;
        }
    }

    if (not has_feasible_group and is_timeout) {
        single_group_ptr = new Group(problem, {order.get_id()});
        best_group = single_group_ptr;
        has_feasible_group = routing(problem, *best_group, now_time, 0, min_slack_time, min_cost_time);
        is_single_group = true;
    }

    if (has_feasible_group) {
        has_feasible_driver = try_dispatch_group(*best_group, is_delay);
        if (has_feasible_driver) {
            best_group->set_how_dispatch(event);
            served_groups.push_back(*best_group);
            if (USE_DISTRIBUTION_PREDICT) {
                for (int dispatched_id : best_group->get_member_ids())
                    add_experience(dispatched_id, best_group, true);
            }
            delete_node_rewrite(*best_group, true);
            return true;
        }
    }
    if (is_single_group)
        delete(single_group_ptr);

    if (not has_feasible_group or not has_feasible_driver) {
        if (is_timeout) {
            if (has_feasible_group and not has_feasible_driver)
                event = NO_DRIVER;
            else
                event = NO_GROUP;
            Group group(problem, {order.get_id()});
            group.set_how_dispatch(event);
            failed_groups.push_back(group);
            if (USE_DISTRIBUTION_PREDICT)
                add_experience(order.get_id(), nullptr, true);
            delete_node_rewrite(group, true);
            return true;
        }
        return false;
    }
}


void ShareabilityGraph::find_cliques(int current_pos, const vector<int>& neighbors,
                                     unordered_set<int>& nodes,
                                     vector<Group>& groups) {
    if (nodes.size() >= CAPACITY_CONSTRAINT) return;

    for (int pos = current_pos + 1; pos < neighbors.size(); pos++) {
        if (nodes.count(neighbors[pos]) > 0 or not has_edge(neighbors[current_pos], neighbors[pos]))
            continue;
        nodes.insert(neighbors[pos]);
        groups.emplace_back(problem, nodes);
        find_cliques(pos, neighbors, nodes, groups);
        nodes.erase(neighbors[pos]);
    }
}


bool ShareabilityGraph::enumerate_group(const Order &order, vector<Group>& groups) {
    vector<int> neighbors;
    neighbors.push_back(order.get_id());
    unordered_set<int> nodes;
    nodes.insert(order.get_id());

    for (auto &kv : rewrite_graph[order.get_id()]) {
        if (transform_to_timeslot(now_time) < kv.second)
            neighbors.push_back(kv.first);
    }
    if (neighbors.size() == 1) {
        return false;
    }
    find_cliques(0, neighbors, nodes, groups);
    return true;
}


bool ShareabilityGraph::try_dispatch_group(Group& group, bool& is_delay) {
    int driver_id, gid = -1;
    double min_wait_delay_time = INF, min_wait_pick_time = INF;
    double min_slack_time = INF, min_cost_time = INF;
    bool flag;
    if (not routing(problem, group, now_time, 0, min_slack_time, min_cost_time))
        return false;
    flag = gi->get_nearest_available_driver_in_neighbors(problem, group, now_time,
                                                         driver_id, gid,
                                                         min_wait_delay_time, min_wait_pick_time);
    if (flag) {
        is_delay = (min_wait_delay_time > 0);
        Driver& driver = problem->get_single_driver(driver_id);
        if (routing(problem, group, now_time, min_wait_delay_time + min_wait_pick_time, min_slack_time, min_cost_time)) {
            group.add_time_to_driver_cost(min_wait_pick_time);
            group.set_pick_cost(min_wait_pick_time);
            serve(problem, driver, group, (now_time + min_wait_delay_time), min_wait_pick_time);
            update_driver_distribution(driver.history_gid[driver.history_gid.size() - 2], driver.history_gid.back());
            if (not gi->get_single_grid(gid).remove_driver(driver_id)) {
                cout << "Error remove driver " << driver_id << " from grid index" << endl;
            }
            gi->get_single_grid(group.get_end_grid_id()).insert_driver(driver_id);
        } else {
            flag = false;
        }

    }

    return flag;
}


bool ShareabilityGraph::update_order_distribution(Group* group, bool is_add) {
    bool ret = false;
    int update_num;
    if (is_add)
        update_num = 1;
    else
        update_num = -1;
    for (int id : group->get_member_ids()) {
        Order& order = problem->get_single_order(id);
        int start_gid, end_gid;
        in_which_grid_id(order.get_pick_location().first, order.get_pick_location().second, start_gid);
        in_which_grid_id(order.get_drop_location().first, order.get_drop_location().second, end_gid);

        start_distribution[start_gid] += update_num;
        end_distribution[end_gid] += update_num;

        ret = true;
    }

    return ret;
}


bool ShareabilityGraph::update_driver_distribution(int start_gid, int end_gid) {
    if (driver_distribution.size() <= start_gid or driver_distribution.size() <= end_gid)
        return false;
    driver_distribution[start_gid] += -1;
    driver_distribution[end_gid] += 1;
    return true;
}


bool ShareabilityGraph::init_driver_distribution(const vector<Driver> &all_drivers) {
    bool ret = false;
    int gid = -1;
    for (auto driver : all_drivers) {
        in_which_grid_id(driver.get_location().first, driver.get_location().second, gid);
        driver_distribution[gid] += 1;
        ret = true;
    }
    return ret;
}


bool ShareabilityGraph::update_heatmap() {
    for (const auto& iter : working_set) {
        Order order = problem->get_single_order(iter);
        pair<int, int> pick_grid;
        pair<int, int> drop_grid;

        in_which_grid(order.get_pick_location().first, order.get_pick_location().second, pick_grid);
        in_which_grid(order.get_drop_location().first, order.get_drop_location().second, drop_grid);

        heatmap[pick_grid.first][pick_grid.second] += 1;
        heatmap[drop_grid.first][drop_grid.second] += 1;
    }

    return true;
}


bool ShareabilityGraph::get_group_feature(const Order& order, vector<int>& feature) {

    int partner_id = best_partner_map[order.get_id()].first;

    if (partner_id == -1) {
        feature = vector<int>(15);
        return false;
    }

    Order partner = problem->get_single_order(partner_id);

    pair<int, int> order_pick_grid, order_drop_grid, partner_pick_grid, partner_drop_grid;

    in_which_grid(order.get_pick_location().first, order.get_pick_location().second, order_pick_grid);
    in_which_grid(order.get_drop_location().first, order.get_drop_location().second, order_drop_grid);
    in_which_grid(partner.get_pick_location().first, partner.get_pick_location().second, partner_pick_grid);
    in_which_grid(partner.get_drop_location().first, partner.get_drop_location().second, partner_drop_grid);

    feature.emplace_back(order_pick_grid.first);
    feature.emplace_back(order_pick_grid.second);
    feature.emplace_back(order_drop_grid.first);
    feature.emplace_back(order_drop_grid.second);
    feature.emplace_back(partner_pick_grid.first);
    feature.emplace_back(partner_pick_grid.second);
    feature.emplace_back(partner_drop_grid.first);
    feature.emplace_back(partner_drop_grid.second);
    feature.emplace_back(heatmap[order_pick_grid.first][order_pick_grid.second]);
    feature.emplace_back(heatmap[order_drop_grid.first][order_drop_grid.second]);
    feature.emplace_back(heatmap[partner_pick_grid.first][partner_pick_grid.second]);
    feature.emplace_back(heatmap[partner_drop_grid.first][partner_drop_grid.second]);
    feature.emplace_back(int(graph[order.get_id()][partner_id][0] / 0.05));
    feature.emplace_back(int((order.get_pick_time() - problem->get_start_time()) / 60));
    feature.emplace_back(max(int((partner.get_pick_time() - order.get_pick_time()) / 60), 0));

    return true;
}


bool ShareabilityGraph::get_all_group_features(vector<vector<int>> &features) {

    for (const auto& iter : working_set) {
        Order order = problem->get_single_order(iter);
        vector<int> feature;
        get_group_feature(order, feature);
        features.push_back(feature);
    }

    return true;
}


bool ShareabilityGraph::update_best_partner_attr_now(const Order& order) {
    vector<double> total_attr_vec(3);
    vector<double> o1_attr(3), o2_attr(3);
    bool ret;
    int partner_id = -1;
    if (get_max_neighbor(order, partner_id, total_attr_vec)) {
        Order partner = problem->get_single_order(partner_id);
        ret = compute_group_attr(order, partner, now_time, total_attr_vec, o1_attr, o2_attr);
        total_attr_vec.resize(7);
        total_attr_vec[3] = (now_time - order.get_pick_time());
        total_attr_vec[4] = (o1_attr[2]);
        total_attr_vec[5] = (now_time - partner.get_pick_time());
        total_attr_vec[6] = (o2_attr[2]);
        best_partner_map[order.get_id()] = make_pair(partner_id, total_attr_vec);
        be_best_partner_map[partner_id].insert(order.get_id());
    } else {
        ret = false;
        best_partner_map[order.get_id()].first = -1;
    }
    return ret;
}


bool ShareabilityGraph::get_best_partner(const Order& order, pair<int, vector<double>>& best_partner) {
    update_best_partner_attr_now(order);
    pair<int, vector<double>> tmp = best_partner_map[order.get_id()];
    best_partner.first = tmp.first;
    for (int i = 0; i < tmp.second.size(); i++)
        best_partner.second[i] = tmp.second[i];
    return true;
}


bool ShareabilityGraph::get_best_group(const Order& order, Group& best_group) {
    bool exist = best_group_map.count(order.get_id()) > 0;
    if (exist)
        best_group = best_group_map[order.get_id()];
    return exist;
}


State ShareabilityGraph::get_current_state(int exist_order_id) {
    int start_gid = -1, end_gid = -1;
    Order& order = problem->get_single_order(exist_order_id);
    in_which_grid_id(order.get_pick_location().first, order.get_pick_location().second, start_gid);
    in_which_grid_id(order.get_drop_location().first, order.get_drop_location().second, end_gid);

    vector<float> start_one_hot(NUM_GRID_X * NUM_GRID_Y, 0);
    vector<float> end_one_hot(NUM_GRID_X * NUM_GRID_Y, 0);

    start_one_hot[start_gid] = 1;
    end_one_hot[end_gid] = 1;

    vector<float> time_and_wait(2, 0);
    time_and_wait[0] = floor((order.get_pick_time() - problem->get_start_time()) / TIMESLOT_DELTA);
    time_and_wait[1] = floor((now_time - order.get_pick_time()) / TIMESLOT_DELTA);

    State s{start_one_hot, start_distribution,
            end_one_hot, end_distribution,
            driver_distribution, time_and_wait};

    return s;
}


void ShareabilityGraph::add_experience(int order_id, Group* group, bool is_terminate) {
    double target_value, reward;
    State s = get_current_state(order_id);
    target_value = problem->get_single_order(order_id).get_original_slack() / 3 * 2; // p - p / 3

    if (is_terminate)
        if (group != nullptr)
            reward = problem->get_single_order(order_id).get_original_slack() - group->detour_time[order_id];
        else
            reward = -1 * TIMESLOT_DELTA;
    else
        reward = -1 * TIMESLOT_DELTA;

    dqn->cumulative_reward += reward;
    dqn->replace_terminate(order_id, s, 0, reward, target_value, is_terminate);
}

#endif //TEST_SHAREABILITY_GRAPH_REWRITE_H



