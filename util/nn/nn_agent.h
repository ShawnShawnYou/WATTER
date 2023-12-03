

#ifndef TEST_NN_AGENT_H
#define TEST_NN_AGENT_H

#include "bits/stdc++.h"
#include <torch/torch.h>
#include "nn_layer.h"

const int replayMemoryCapacity = 10000;  // 经验回放的容量
const int batchSize = 64;  // 批量大小
const int targetUpdateFreq = 10;  // 更新目标网络的频率
const int numEpisodes = 50;  // 训练的总回合数
const string model_root = "../file/grid_model/";

struct State {
    vector<float> startOneHot;
    vector<float> startDistribution;
    vector<float> endOneHot;
    vector<float> endDistribution;
    vector<float> driverDistribution;
    vector<float> timeAndWait;
};


struct Experience {
    State current_state;
    State next_state;
    int action;
    double reward;
    double target_value;
    bool done;
};


class DQNAgent {
private:
    double learning_rate = 0.001;
    double gamma = 1;
    double omega = 0.7;
    int grid_size = 10;

    std::shared_ptr<VN> mainNet;
    std::shared_ptr<VN> targetNet;
    std::shared_ptr<torch::optim::Adam> optimizer;

    std::vector<Experience> replayMemory;  // 经验回放的经验存储
    int currentMemoryIndex = 0;            // 当前存储经验的索引
    int currentMemorySize = 0;             // 当前存储经验的数量
    std::unordered_map<int, Experience> experience_buffer;

    std::shared_ptr<torch::Device> device;

    string model_path;

public:
    double cumulative_reward = 0;

    DQNAgent(string save_city, string algo, double save_ddl, bool use_target_value,
             int train_mode, int train_round, int grid_size);

    // 更新目标网络
    void updateTargetNetwork();

    // 存储经验
    void storeExperience(const Experience& experience);

    // 经验回放
    float experienceReplay();

    void train();

    void replace_terminate(int id, State& state, int action, double reward, double target_value, bool is_terminate);

    double estimate(State state);

    torch::Tensor estimate_batch(vector<State>& states, bool is_main_net, bool is_train);
};


#endif //TEST_NN_AGENT_H
