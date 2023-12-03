#include "problem.h"

ProblemInstance::ProblemInstance() {
    string DATA_ROOT;
    #ifdef __linux__
        DATA_ROOT = "";
        #define FOLDER_SEPARATOR "/"
    #elif _WIN32
        DATA_ROOT = R"";
        #define FOLDER_SEPARATOR "\\"
    #endif
    vector<string> CITY = {"chengdu", "nyc", "xian", "test"};

    for (const string& city_name : CITY){
        string city_root = DATA_ROOT + city_name + std::string(FOLDER_SEPARATOR) + city_name;
        data_path[city_name] = city_root + "_order.txt";
        vertex_path[city_name] = city_root + ".node";
    }

    bool read_data_new_way = true;
    if (not read_data_new_way) {
        vector<vector<string>> csv_arr = data_loader();

        vector<string> first_order_str = csv_arr[0];
        first_order_str[0] = to_string(0);
        Order first_order(first_order_str, 0);
        start_time = first_order.get_pick_time();

        int counter = 0;
        for (vector<string> order_str : csv_arr) {
            order_str[0] = to_string(counter);
            Order* order_ptr = new Order(order_str, start_time);
            all_orders.push_back(*order_ptr);
            delete(order_ptr);
            counter++;
        }
    } else {
        load_data_from_vertex();
    }

    start_time = all_orders[0].get_pick_time();
    end_time = start_time + 60 * 60 * 26;
    iter = all_orders.begin();
}


bool ProblemInstance::load_data_from_vertex() {
    string city_data_path = data_path[city];
    string city_vertex_path = vertex_path[city];

    ifstream vertexFile;
    vertexFile.open(city_vertex_path.c_str());
    if (!vertexFile.is_open()) {
        printf("%s does not exist\n", city_vertex_path.c_str());
        exit(0);
    }
    int vertex_size;  vertexFile >> vertex_size;
    vertices.resize(vertex_size);
    double mercator_x, mercator_y;
    double mxx = -1 * INF, mnx = INF, mxy = -1 * INF, mny = INF;
    for(int i = 0; i < vertex_size; ++i) {
        vertexFile >> mercator_x >> mercator_y;
        if (city == "nyc")
            vertices[i] = MercatorToGps(mercator_x, mercator_y);
        else
            vertices[i] = make_pair(mercator_x, mercator_y);

        if (vertices[i].first > mxx) mxx = vertices[i].first;
        if (vertices[i].first < mnx) mnx = vertices[i].first;
        if (vertices[i].second > mxy) mxy = vertices[i].second;
        if (vertices[i].second < mny) mny = vertices[i].second;

    }
    XREGION = make_pair(mnx, mxx);
    YREGION = make_pair(mny, mxy);
    vertexFile.close();


    vector<time_t> ref_pick_time;
    if (city == "chengdu") {
        ifstream ref_ifs;
        ref_ifs.open(ref_request_file.c_str());
        if (!ref_ifs.is_open()) {
            printf("%s does not exist\n", ref_request_file.c_str());
            exit(0);
        }
        int data_size; ref_ifs >> data_size;
        data_size = min(REQUEST_SIZE * 3, data_size);

        time_t pick_time;
        double tmp_s, tmp_e, tmp_com;
        for (int i = 0; i < data_size; i++) {
            ref_ifs >> pick_time >> tmp_s >> tmp_e >> tmp_com;
            ref_ifs >> pick_time >> tmp_s >> tmp_e >> tmp_com;
            ref_pick_time.push_back(pick_time);
        }
        ref_ifs.close();
    }
    std::sort(ref_pick_time.begin(), ref_pick_time.end());

    ifstream ifs;
    ifs.open(city_data_path.c_str());
    if (!ifs.is_open()) {
        printf("%s does not exist\n", city_data_path.c_str());
        exit(0);
    }
    int data_size; ifs >> data_size;
    data_size = min(REQUEST_SIZE, data_size); // 最多读取50W条数据
    int order_id = 0;
    time_t first_pick_time;
    while (order_id < data_size) {
        time_t pick_time;
        int s, e, num;
        ifs >> pick_time >> s >> e >> num;
        if (order_id == 0)
            first_pick_time = pick_time;
        if (pick_time < first_pick_time)
            first_pick_time = pick_time;
        all_orders.emplace_back(order_id, pick_time, vertices[s], vertices[e]);
        order_id++;
    }
    ifs.close();

    for (int id = 0; id < data_size; id++)
        all_orders[id].set_pick_time(all_orders[id].get_pick_time() - first_pick_time);

    std::sort(all_orders.begin(), all_orders.end(), [](const Order& a, const Order& b) {
        return a.get_pick_time() < b.get_pick_time();
    });

    order_id = 0;
    while (order_id < data_size) {
        all_orders[order_id].set_id(order_id);
        order_id++;
    }

}

pair<double, double> ProblemInstance::MercatorToGps(double j, double w) {
    double jing = j / 20037508.34 * 180;
    double ly = w / 20037508.34 * 180;
    double wei = 180 / M_PI * (2 * atan(exp(ly * M_PI / 180)) - M_PI / 2);
    return make_pair(jing, wei);
}


bool ProblemInstance::valid_location(const string& pick_x, const string& pick_y, const string& drop_x, const string& drop_y) {
    // pick == 0 or drop == 0 or pick == drop
    // pick, drop in region
    if (pick_x == "0" or pick_y == "0" or drop_x == "0" or drop_y == "0" )
        return false;

    if (pick_x == drop_x and pick_y == drop_y)
        return false;

    if (XREGION.first <= stod(pick_x) and stod(pick_x) <= XREGION.second and
        YREGION.first <= stod(pick_y) and stod(pick_y) <= YREGION.second and
        XREGION.first <= stod(drop_x) and stod(drop_x) <= XREGION.second and
        YREGION.first <= stod(drop_y) and stod(drop_y) <= YREGION.second)
        return true;

    return false;
}



vector<vector<string>>& ProblemInstance::data_loader() {
    ifstream csv_reader(ORDER_DATA_DIR + ORDER_DATA_FILE);
    string line;
    auto csv_arr = new vector<vector<string>>();

    while (getline(csv_reader, line)) {
        stringstream ss(line);
        string word;
        vector<string> line_arr;
        while (getline(ss, word, ',')) {
            line_arr.push_back(word);
        }
        if (not valid_location(line_arr[5], line_arr[6], line_arr[9], line_arr[10]))
            continue;
        csv_arr->push_back(line_arr);
    }

    csv_reader.close();

    return *csv_arr;
}

vector<Order>& ProblemInstance::batch(time_t now_time) {
    auto batch_orders = new vector<Order>();
    while (iter != all_orders.end()) {
        if (iter->get_pick_time() > now_time) {
            break;
        }
        batch_orders->push_back(*iter);
        iter++;
    }
    return *batch_orders;
}


bool ProblemInstance::init_workers() {
    srand((unsigned)time(NULL));
    int random_order_id;
    for (int i = 0; i < WORKER_NUM; i++) {
        random_order_id = rand() % (all_orders.size());
        pair<double, double> location = all_orders[random_order_id].get_pick_location();
        order_to_driver.push_back(random_order_id);
        all_drivers.emplace_back(i, location);
    }

    return true;
}


bool ProblemInstance::save_problem() {
    ofstream vertex_ofs(vertex_file);
    ofstream request_ofs(request_file);
    ofstream data_ofs(data_file);

    int node_id = 0;
    vertex_ofs << all_orders.size() * 2 << endl;
    request_ofs << all_orders.size() << endl;
    for (auto order : all_orders) {
        vertex_ofs << order.get_pick_location().first << " " << order.get_pick_location().second << endl;
        vertex_ofs << order.get_drop_location().first << " " << order.get_drop_location().second << endl;

        request_ofs << order.get_pick_time() - start_time << " ";
        request_ofs << node_id << " " << node_id + 1 << " " << "1" << endl;
        node_id += 2;
    }

    data_ofs << all_drivers.size() << " " << CAPACITY_CONSTRAINT << " "  << 1000 << " "  << ALPHA << endl;
    for (auto driver : all_drivers) {
        int pick_node_id = order_to_driver[driver.get_id()] * 2;
        data_ofs << pick_node_id << " " << CAPACITY_CONSTRAINT << endl;
    }
    data_ofs << "600" << " " << "10";

    vertex_ofs.close();
    request_ofs.close();
    data_ofs.close();
    return false;
}


Order& ProblemInstance::get_single_order(int order_id) {
    return all_orders[order_id];
};


Driver& ProblemInstance::get_single_driver(int driver_id) {
    return all_drivers[driver_id];
}
