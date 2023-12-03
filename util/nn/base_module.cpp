#include "base_module.h"

ConvReluBnImpl::ConvReluBnImpl(int input_channel, int output_channel, int kernel_size, int stride) {
    conv = register_module("conv", torch::nn::Conv2d(conv_options(input_channel,output_channel,kernel_size,stride,kernel_size/2)));
    bn = register_module("bn", torch::nn::BatchNorm2d(output_channel));
    pool = register_module("pool", torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions(2).stride(2)));
//
//    torch::nn::init::kaiming_normal_(conv->weight);
//    torch::nn::init::constant_(conv->bias, 0.0);
    torch::nn::init::normal_(bn->weight, 0.0, 0.02);
    torch::nn::init::constant_(bn->bias, 0.1);

    conv->to(torch::kCUDA);
    bn->to(torch::kCUDA);
    pool->to(torch::kCUDA);
}

torch::Tensor ConvReluBnImpl::forward(torch::Tensor x) {
    x = conv->forward(x);
    x = torch::relu(x);
    x = pool(x);
    x = bn(x);
    return x;
}

LinearBnReluImpl::LinearBnReluImpl(int in_features, int out_features){
    ln = register_module("ln", torch::nn::Linear(torch::nn::LinearOptions(in_features, out_features)));
    bn = register_module("bn", torch::nn::BatchNorm1d(out_features));

    torch::nn::init::kaiming_normal_(ln->weight);
    torch::nn::init::constant_(ln->bias, 0.1);

    ln->to(torch::kCUDA);
    bn->to(torch::kCUDA);
}

torch::Tensor LinearBnReluImpl::forward(torch::Tensor x){
    x = torch::relu(ln->forward(x));
//    x = bn(x);
    return x;
}

