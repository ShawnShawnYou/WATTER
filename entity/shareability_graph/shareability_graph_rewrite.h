
#ifndef E_RIDE_TEST_SHAREABILITY_GRAPH_REWRITE_H
#define E_RIDE_TEST_SHAREABILITY_GRAPH_REWRITE_H
#include "../../util/setting.h"
#include "../../util/util.h"
#include "../grid/grid_index.h"
#include "../problem/problem.h"
#include "../../util/nn/nn_agent.h"

enum RiderEvent {
    ARRIVE, TIMEOUT, POLICY, CHECK, CLEAR, DELAY, NO_GROUP, NO_DRIVER,
};

static const string EnumStrings[] = {
        "ARRIVE", "TIMEOUT", "POLICY", "CHECK", "CLEAR", "DELAY", "NO_GROUP", "NO_DRIVER"
};


extern ProblemInstance* problem;
extern GridIndex* gi;
extern DQNAgent* dqn;


class ShareabilityGraph {
private:
    unordered_map<int, unordered_map<int, vector<double>>> graph;

    unordered_map<int, unordered_map<int, int>> rewrite_graph;

    unordered_set<int> working_set;

    unordered_map<int, pair<int, vector<double>>> best_partner_map;

    unordered_map<int, unordered_set<int>> be_best_partner_map;

    unordered_map<int, Group> best_group_map;

    unordered_map<int, unordered_set<int>> be_best_group_map;

    unordered_map<int, unordered_map<int, unordered_set<int>> > edge_expire_index;

    vector<Group> to_be_served_groups;
    vector<Group> served_groups;
    vector<Group> failed_groups;

    vector<vector<int>> heatmap;
    vector<float> driver_distribution;
    vector<float> start_distribution;
    vector<float> end_distribution;

    // setting
    bool is_sorted_max;

    int sorted_attr_index = 2;

    virtual bool get_max_neighbor(const Order& order, int& max_id, vector<double>& revenue_attr);



public:
    time_t now_time;
    time_t init_timestamp;
    time_t total_max_routing_cost_time = 0;
    time_t parallel_save_time = 0;
    double ans = 0;
    double total_len = 0;
    double count_neighbor_arrive = 0;
    double count_neighbor_leave = 0;
    vector<double> extra_time_sample_set;
    unordered_map<int, double> detour_map;

    ShareabilityGraph();

    unordered_set<int>& get_exist_orders() { return working_set; }

    vector<Group>& get_to_be_served_groups() { return to_be_served_groups; }

    vector<Group>& get_served_groups() { return served_groups; }

    vector<Group>& get_failed_groups() { return failed_groups; }

    bool insert_edge(int o1_id, int o2_id, time_t expire_timestamp);

    bool has_edge(int o1_id, int o2_id);

    bool add_node(const Order& order);

    bool add_node_rewrite(const Order& order);

    void find_cliques(int current_pos, const vector<int>& neighbors,
                      unordered_set<int>& nodes,
                      vector<Group>& results);

    bool enumerate_group(const Order& order, vector<Group>& groups);

    bool update_best_group(const Order& order);

    bool delete_node(const Order& order);

    bool delete_node_rewrite(Group& group, bool is_departure);

    bool trigger_event_rewrite(const Order& order, RiderEvent event);

    bool dispatch_rewrite(const Order& order, bool is_timeout, RiderEvent event);

    bool try_dispatch_group(Group& group, bool& is_delay);

    bool update_order_distribution(Group* group, bool is_add);

    bool update_driver_distribution(int start_gid, int end_gid);

    bool init_driver_distribution(const vector<Driver> &all_drivers);

    vector<float>& get_start_distribution() { return start_distribution; }
    vector<float>& get_end_distribution() { return end_distribution; }
    vector<float>& get_driver_distribution() { return driver_distribution; }

    bool update_heatmap();

    bool get_group_feature(const Order& order, vector<int>& feature);

    bool get_all_group_features(vector<vector<int>>& features);

    bool get_best_partner(const Order& order, pair<int, vector<double>>& best_partner);

    bool get_best_group(const Order& order, Group& best_group);

    bool update_best_partner_attr_now(const Order& order);

    int transform_to_timeslot(time_t timestamp) { return (timestamp - init_timestamp) / TIMESLOT_DELTA; }

    unordered_map<int, unordered_set<int>>& get_expired_edges(int timeslot) { return edge_expire_index[timeslot]; }

    State get_current_state(int order_id);

    void add_experience(int order_id, Group* group, bool is_terminate);

};

#endif //E_RIDE_TEST_SHAREABILITY_GRAPH_REWRITE_H