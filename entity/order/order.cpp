

#include "order.h"

time_t string2timestamp(string str){
    struct tm tm_;
    int year, month, day, hour, minute,second;
    sscanf(str.c_str(),"%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    tm_.tm_year  = year-1900;
    tm_.tm_mon   = month-1;
    tm_.tm_mday  = day;
    tm_.tm_hour  = hour;
    tm_.tm_min   = minute;
    tm_.tm_sec   = second;
    tm_.tm_isdst = 0;

    time_t timeStamp = mktime(&tm_);
    return timeStamp;
}

Order::Order() {
    speed = UNI_SPEED;
    passenger_count = 1;
}


Order::Order(vector<string> order_str, time_t start_time) {

    id = stoi(order_str[0]);

    pick_time = string2timestamp(order_str[1]) - start_time;

//    passenger_count = stoi(order_str[3]);
    passenger_count = 1;

    pick_x = stod(order_str[5]);
    pick_y = stod(order_str[6]);

    try {
        drop_x = stod(order_str[9]);
        drop_y = stod(order_str[10]);
    } catch (exception& e) {
        drop_x = pick_x;
        drop_y = pick_y;
    }

    absolute_distance = abs(pick_x - drop_x) + abs(pick_y - drop_y);

    speed = UNI_SPEED;

    if (DEADLINE_GAMMA < 20) {
        deadline = pick_time + DEADLINE_GAMMA * (absolute_distance / speed);
    } else {
        deadline = pick_time + DEADLINE_GAMMA + (absolute_distance / speed);
    }
}

Order::Order(int order_id, time_t pick_t, pair<double, double> pick_location, pair<double, double> drop_location) {
    id = order_id;

    pick_time = pick_t;

    //    passenger_count = stoi(order_str[3]);
    passenger_count = 1;

    pick_x = pick_location.first;
    pick_y = pick_location.second;

    drop_x = drop_location.first;
    drop_y = drop_location.second;

    absolute_distance = abs(pick_x - drop_x) + abs(pick_y - drop_y);

    speed = UNI_SPEED;

    recompute_ddl();
}

void Order::recompute_ddl() {
    if (DEADLINE_GAMMA < 20) {
        deadline = pick_time + DEADLINE_GAMMA * (absolute_distance / speed);
    } else {
        deadline = pick_time + DEADLINE_GAMMA + (absolute_distance / speed);
    }
}


int Order::get_durable() {
    return int(this->get_original_slack() * DURABLE_BASE);
}