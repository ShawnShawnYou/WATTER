#ifndef E_RIDE_TEST_PROBLEM_INSTANCE_H
#define E_RIDE_TEST_PROBLEM_INSTANCE_H
#include "../order/order.h"
//#include "../order/group.h"
#include "../driver/driver.h"
//#include "../grid/grid_index.h"
#include "../grid/grid.h"
#include "../util/setting.h"


class ProblemInstance {
private:
    string directory = "../file";
    string root = directory + "/data" + "/" + city + "/" + city;
    string vertex_file = root + ".node";
    string edge_file = root + ".edge";
    string path_label_file = root + ".label";
    string order_file = root + ".order";
    string data_file = root + "_taxi.txt";
    string request_file = root + "_order.txt";
    #ifdef __linux__
    string ref_request_file = "";
    #elif _WIN32
    string ref_request_file = R"(E:\workspace\A-Experiment-dataset\ridesharing_dataset\data\test\test_order.txt)";
    #endif

    unordered_map<string, string> vertex_path;
    unordered_map<string, string> data_path;

    time_t start_time;
    time_t end_time;

    vector<Order>::iterator iter;

    vector<Order> all_orders;

    vector<Driver> all_drivers;

    vector<int> order_to_driver;

    vector<vector<string>>& data_loader();

    vector<pair<double, double>> vertices;

    bool load_data_from_vertex();

    pair<double, double> MercatorToGps(double j, double w);


public:
    ProblemInstance();
    time_t get_start_time() const { return this->start_time; };
    time_t get_end_time() const { return this->end_time; };

    const vector<Order>& get_all_orders() const { return all_orders; };
    const vector<Driver>& get_all_drivers() const { return all_drivers; }
    Order& get_single_order(int order_id);
    Driver& get_single_driver(int driver_id);

    bool init_workers();

    bool valid_location(const string& pick_x, const string& pick_y, const string& drop_x, const string& drop_y);

    vector<Order>& batch(time_t now_time);

    bool reset_batch_iter() { iter = all_orders.begin(); return true; }

    bool save_problem();

};

#endif //E_RIDE_TEST_PROBLEM_INSTANCE_H