#include "nn_layer.h"

VN::VN(int grid_size){

    _grid_size = grid_size;

    fc1 = LinearBnRelu(2, 32);
    fc2 = LinearBnRelu(32, 128);
    
    
    int fc3_input_size = 512;
    switch (_grid_size) {
        case 5:
            fc3_input_size = 224;
            break;
        case 10:
            fc3_input_size = 512;
            break;
        case 15:
            fc3_input_size = 992;
            break;
        case 20:
            fc3_input_size = 2528;
            break;
        case 25:
            fc3_input_size = 3584;
            break;
        case 30:
            fc3_input_size = 4832;
            break;
    }
    
    fc3 = LinearBnRelu(fc3_input_size, 128);
    fc4 = LinearBnRelu(128, 1);

    conv1 = ConvReluBn(2, 32, 3);
    conv2 = ConvReluBn(32, 64, 3);
    conv3 = ConvReluBn(2, 32, 3);
    conv4 = ConvReluBn(32, 64, 3);
    conv5 = ConvReluBn(1, 32, 3);
    conv6 = ConvReluBn(32, 64, 3);

    out_conv1 = torch::nn::Conv2d(conv_options(64, 32, 3));
    out_conv1->to(torch::kCUDA);

    out_conv2 = torch::nn::Conv2d(conv_options(64, 32, 3));
    out_conv2->to(torch::kCUDA);

    out_conv3 = torch::nn::Conv2d(conv_options(64, 32, 3));
    out_conv3->to(torch::kCUDA);

    // --------------------register----------------------------
    fc1 = register_module("fc1", fc1);
    fc2 = register_module("fc2", fc2);
    fc3 = register_module("fc3", fc3);
    fc4 = register_module("fc4", fc4);

    conv1 = register_module("conv1", conv1);
    conv2 = register_module("conv2", conv2);
    conv3 = register_module("conv3", conv3);
    conv4 = register_module("conv4", conv4);
    conv5 = register_module("conv5", conv5);
    conv6 = register_module("conv6", conv6);

    out_conv1 = register_module("out_conv1", out_conv1);
    out_conv2 = register_module("out_conv2", out_conv2);
    out_conv3 = register_module("out_conv3", out_conv3);


}

torch::Tensor VN::forward(torch::Tensor startOneHot, torch::Tensor startDistribution,
                          torch::Tensor endOneHot, torch::Tensor endDistribution,
                          torch::Tensor driverDistribution,
                          torch::Tensor timeAndWait) {
    startDistribution = (startDistribution - torch::mean(startDistribution)) / torch::norm(startDistribution, 2, 1).unsqueeze(1);
    endDistribution = (endDistribution - torch::mean(endDistribution)) / torch::norm(endDistribution, 2, 1).unsqueeze(1);
    driverDistribution = (driverDistribution - torch::mean(driverDistribution)) / torch::norm(driverDistribution, 2, 1).unsqueeze(1);

    torch::Tensor startOneHotMatrix = startOneHot.view({-1, 1, _grid_size, _grid_size});
    torch::Tensor endOneHotMatrix = endOneHot.view({-1, 1, _grid_size, _grid_size});

    torch::Tensor startDistributionMatrix = startDistribution.view({-1, 1, _grid_size, _grid_size});
    torch::Tensor endDistributionMatrix = endDistribution.view({-1, 1, _grid_size, _grid_size});
    torch::Tensor driverDistributionMatrix = driverDistribution.view({-1, 1, _grid_size, _grid_size});

    int batch_size = startOneHotMatrix.size(0);

    auto combined_start_matrix = torch::cat({startOneHotMatrix, startDistributionMatrix}, 1);
    auto start_info = out_conv1->forward(conv2->forward(conv1->forward(combined_start_matrix))).view({batch_size, -1});

    auto combined_end_matrix = torch::cat({endOneHotMatrix, endDistributionMatrix}, 1);
    auto end_info = out_conv2->forward(conv4->forward(conv3->forward(combined_end_matrix))).view({batch_size, -1});

    auto driver_info = out_conv3->forward(conv6->forward(conv5->forward(driverDistributionMatrix))).view({batch_size, -1});

    auto time_info = fc2->forward(fc1->forward(timeAndWait));

    torch::Tensor combined = torch::cat({start_info, end_info, driver_info, time_info}, 1);

    auto x = fc4->forward(fc3->forward(combined));

    return x;
}


