
#ifndef TEST_GRID_H
#define TEST_GRID_H
#include "../../util/setting.h"

class Grid {
private:
    int id;
    unordered_set<int> order_set;
    unordered_set<int> driver_set;

public:
    Grid(int gid);

    unordered_set<int>& get_order_set() { return order_set; }
    unordered_set<int>& get_driver_set() { return driver_set; }

    bool insert_order(int order_id) { order_set.insert(order_id); return true; }
    bool insert_driver(int driver_id) { driver_set.insert(driver_id); return true;  }

    bool remove_order(int order_id) {
        bool ret = order_set.count(order_id) > 0;
        if (ret)
            order_set.erase(order_id);
        return ret;
    }

    bool remove_driver(int driver_id) {
        bool ret = driver_set.count(driver_id) > 0;
        if (ret)
            driver_set.erase(driver_id);
        return ret;
    }



};




#endif //TEST_GRID_H
