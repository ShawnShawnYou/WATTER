#include "nn_agent.h"

std::string formatDouble(double value) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << value;
    return stream.str();
}

bool fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}


DQNAgent::DQNAgent(string save_city, string algo, double save_ddl, bool use_target_value,
                   int train_mode, int train_round, int grid_size) {
    model_path = model_root + to_string(grid_size) + "_" +
            to_string(train_mode) + "_" + to_string(train_round) + "_" + save_city + "_" + algo + "_" + formatDouble(save_ddl) + ".pt";
    if (not use_target_value)
        omega = 0;

    // 创建主网络和目标网络
    mainNet = std::make_shared<VN>(grid_size);
    if (fileExists(model_path)) {
        torch::load(mainNet, model_path);
    } else {
        torch::save(mainNet, model_path);
    }

    targetNet = std::make_shared<VN>(*mainNet);
    targetNet->eval();

    // 创建优化器
    optimizer = std::make_shared<torch::optim::Adam>(mainNet->parameters(), torch::optim::AdamOptions(learning_rate));
    device = std::make_shared<torch::Device>(torch::kCUDA, 0);

    // 初始化经验回放的经验存储
    replayMemory.resize(replayMemoryCapacity);
}

torch::Tensor to_matrix(vector<vector<float>>& data) {
    // 获取行和列的数量
    int rows = data.size();
    int cols = data[0].size();

    // 创建张量
    torch::Tensor tensor = torch::zeros({rows, cols});

    // 填充张量
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            tensor[i][j] = data[i][j];
        }
    }

    return tensor;
}


float DQNAgent::experienceReplay() {
    if (currentMemorySize < batchSize)
        return 0;

    mainNet->train();
    mainNet->to(*device);
    targetNet->to(*device);

    // 从经验回放中随机采样一个批次的经验
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> randIndex(0, currentMemorySize - 1);
    std::vector<int> batchIndices(batchSize);
    for (int i = 0; i < batchSize; ++i) {
        batchIndices[i] = randIndex(gen);
    }


    // -----------declaration----------------
    // batch vectors: states
    std::vector<State> current_states;
    std::vector<State> next_states;

    // batch vectors: others
    std::vector<int> batchActions;
    std::vector<double> batchRewards;
    std::vector<double> batchTargets;
    std::vector<bool> batchDones;

    // -----------initialization----------------
    for (int i = 0; i < batchSize; ++i) {
        const Experience& experience = replayMemory[batchIndices[i]];
        // batch vectors: current state
        current_states.push_back(experience.current_state);
        next_states.push_back(experience.next_state);

        // batch vectors: others
        batchActions.push_back(experience.action);
        batchRewards.push_back(experience.reward);
        batchTargets.push_back(experience.target_value);
        batchDones.push_back(experience.done);
    }

    // batch tensor: others
    torch::Tensor batchActionTensor = torch::tensor(batchActions).to(*device);
    torch::Tensor batchRewardTensor = torch::tensor(batchRewards).unsqueeze(1).to(*device);
    torch::Tensor batchTargetTensor = torch::tensor(batchTargets).unsqueeze(1).to(*device);
    std::vector<int> batchDonesInt(batchDones.begin(), batchDones.end());
    torch::Tensor batchDoneTensor = torch::tensor(batchDonesInt).to(*device);
    torch::Tensor onesTensor = torch::ones_like(batchDoneTensor, torch::kFloat).to(*device);
    batchDoneTensor = (onesTensor - batchDoneTensor.to(torch::kFloat)).unsqueeze(1);

    // -----------computation----------------
    // 计算当前状态的 Q 值
    torch::Tensor qValues = estimate_batch(current_states, true, true).to(*device);
    // 计算下一个状态的 Q 值的最大值（使用目标网络）
    torch::Tensor nextQValues = estimate_batch(next_states, false, false).to(*device);
    mainNet->train();
    // 计算 Q 值的目标值
    torch::Tensor targetQValues = batchRewardTensor + gamma * nextQValues * batchDoneTensor;

    // 计算 TD 误差作为损失
    torch::Tensor loss_td = torch::mse_loss(qValues, targetQValues);

//    std::cout << qValues << std::endl;
//    torch::Tensor zero_tensor = torch::zeros_like(qValues);
//    torch::Tensor denominator = torch::where(qValues == zero_tensor, torch::ones_like(qValues), qValues);
//    torch::Tensor relative_error = (qValues - targetQValues) / denominator;
//    torch::Tensor squared_relative_error = torch::pow(relative_error, 2);
//    torch::Tensor percentage_mse = torch::mean(squared_relative_error);
    auto ret = loss_td.item<float>();

    torch::Tensor loss_re = torch::mse_loss(qValues, batchTargetTensor);
//
//    std::cout << qValues.transpose(1, 0) << std::endl;
//    std::cout << targetQValues.transpose(1, 0)  << std::endl;
//    std::cout << batchTargetTensor.transpose(1, 0)  << std::endl;

    torch::Tensor total_loss = omega * loss_re + (1 - omega) * loss_td;

    optimizer->zero_grad();
    total_loss.backward();
    optimizer->step();
    return ret;
}


void DQNAgent::storeExperience(const Experience &experience) {
    replayMemory[currentMemoryIndex] = experience;
    currentMemoryIndex = (currentMemoryIndex + 1) % replayMemoryCapacity;
    if (currentMemorySize < replayMemoryCapacity)
        currentMemorySize++;
}


void DQNAgent::updateTargetNetwork() {
    targetNet = std::make_shared<VN>(*mainNet);
    targetNet->eval();
}


void DQNAgent::replace_terminate(int id, State& state, int action, double reward, double target_value, bool is_terminate) {
    // 如果被其他人带走了？ 所以wait要调用一次，dispatch要对所有group内的订单调用一次，过期要调用一次
    Experience e = {state, state, action, reward, target_value, is_terminate};
    if (experience_buffer.count(id) != 0) {
        Experience& old_e = experience_buffer[id];
        old_e.next_state = state;
        storeExperience(old_e);
        experience_buffer.erase(id);
    }

    if (is_terminate) {
        storeExperience(e);
        experience_buffer.erase(id);
    }
    else
        experience_buffer.insert({id, e});
}


void DQNAgent::train() {
    torch::load(mainNet, model_path);
    updateTargetNetwork();

    int numSteps = 0;  // 当前的步数

    // 训练循环
    double loss = 0;
    for (int i = 0; i < numEpisodes; i++) {
        // 执行动作并收集经验
        // ...

        // 更新主网络的参数
        // ...

        // 每隔一定步数，更新目标网络的参数
        if (numSteps % targetUpdateFreq == 0) {
            updateTargetNetwork();
        }

        // 经验回放
        loss += experienceReplay();

        // 其他训练步骤
        // ...

        numSteps++;
    }

    std::cout << "    reward: " << std::fixed << std::setprecision(4) << std::setw(20) << cumulative_reward;
    std::cout << "    loss:   " << std::fixed << std::setprecision(4) << std::setw(10) << loss / (double)(numEpisodes * batchSize) << endl;

    torch::save(mainNet, model_path);
}


double DQNAgent::estimate(State state) {
    std::vector<State> states(0);
    states.push_back(state);
    torch::Tensor result = estimate_batch(states, true, false);
    double ret = result.item<double>();
    return ret;
}


torch::Tensor DQNAgent::estimate_batch(vector<State>& states, bool is_main_net, bool is_train) {
    std::vector<vector<float>> batch_start_one_hot;
    std::vector<vector<float>> batch_start_distribution;
    std::vector<vector<float>> batch_end_one_hot;
    std::vector<vector<float>> batch_end_distribution;
    std::vector<vector<float>> batch_driver_distribution;
    std::vector<vector<float>> batch_time_and_wait;

    for (auto s : states) {
        batch_start_one_hot.push_back(s.startOneHot);
        batch_start_distribution.push_back(s.startDistribution);
        batch_end_one_hot.push_back(s.endOneHot);
        batch_end_distribution.push_back(s.endDistribution);
        batch_driver_distribution.push_back(s.driverDistribution);
        batch_time_and_wait.push_back(s.timeAndWait);
    }

    torch::Tensor batch_start_one_hot_tensor = to_matrix(batch_start_one_hot).to(*device);
    torch::Tensor batch_start_distribution_tensor = to_matrix(batch_start_distribution).to(*device);
    torch::Tensor batch_end_one_hot_tensor = to_matrix(batch_end_one_hot).to(*device);
    torch::Tensor batch_end_distribution_tensor = to_matrix(batch_end_distribution).to(*device);
    torch::Tensor batch_driver_distribution_tensor = to_matrix(batch_driver_distribution).to(*device);
    torch::Tensor batch_time_and_wait_tensor = to_matrix(batch_time_and_wait).to(*device);

    torch::Tensor result;
    if (is_main_net) {
        if (is_train)
            mainNet->train();
        else
            mainNet->eval();
        result = mainNet->forward(batch_start_one_hot_tensor, batch_start_distribution_tensor,
                                       batch_end_one_hot_tensor, batch_end_distribution_tensor,
                                       batch_driver_distribution_tensor, batch_time_and_wait_tensor).to(torch::kCPU);
    } else {
        targetNet->eval();
        result = targetNet->forward(batch_start_one_hot_tensor, batch_start_distribution_tensor,
                                       batch_end_one_hot_tensor, batch_end_distribution_tensor,
                                       batch_driver_distribution_tensor, batch_time_and_wait_tensor).to(torch::kCPU);
    }

    return result;
}



