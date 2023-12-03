
#ifndef VN_H
#define VN_H
#include "bits/stdc++.h"
using namespace std;
#include "base_module.h"

class VN : public torch::nn::Module{
public:
    VN(int grid_size);

    torch::Tensor forward(torch::Tensor startOneHot, torch::Tensor startDistribution,
                          torch::Tensor endOneHot, torch::Tensor endDistribution,
                          torch::Tensor driverDistribution,
                          torch::Tensor timeAndWait);

private:
    int _grid_size = 10;

    LinearBnRelu fc1{nullptr}, fc2{nullptr};
    LinearBnRelu fc3{nullptr}, fc4{nullptr};
    ConvReluBn conv1{nullptr}, conv2{nullptr}, conv3{nullptr}, conv4{nullptr}, conv5{nullptr}, conv6{nullptr};

    torch::nn::Conv2d out_conv1{nullptr}, out_conv2{nullptr}, out_conv3{nullptr};
};

#endif //VN_H

