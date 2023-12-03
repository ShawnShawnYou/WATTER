#ifndef TEST_GAUSSIAN_H
#define TEST_GAUSSIAN_H

#include "bits/stdc++.h"
using namespace std;

struct Gaussian {
    double mean;
    double variance;
};


void initialize_random(std::vector<Gaussian>& gaussians, int k) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_mean(0.0, 1.0);
    std::uniform_real_distribution<> dis_variance(0.1, 1.0);

    gaussians.resize(k);
    for (int i = 0; i < k; ++i) {
        gaussians[i].mean = dis_mean(gen);
        gaussians[i].variance = dis_variance(gen);
    }
}


double gaussian_pdf(double x, double mean, double variance) {
    // Check if variance is zero or negative
    if (variance <= 0.0) {
        // Handle the case when variance is zero or negative
        // You can choose to return a default value or handle it differently based on your requirements
        return 0.0;  // or any other appropriate value
    }

    // Calculate the coefficient and exponent
    double coeff = 1.0 / std::sqrt(2.0 * M_PI * variance);
    double exponent = -0.5 * std::pow((x - mean) / std::sqrt(variance), 2);

    // Check if exponent is too large
    if (exponent < std::log(DBL_MIN)) {
        // Handle the case when exponent is too large
        // You can choose to return a default value or handle it differently based on your requirements
        return 0.0;  // or any other appropriate value
    }

    // Calculate the result using the modified values
    return coeff * std::exp(exponent);
}


double gaussian_cdf(double x, double mean, double variance) {
    return 0.5 * (1.0 + std::erf((x - mean) / (std::sqrt(2.0 * variance))));
}


double objective_function(double x, const std::vector<double>& means, const std::vector<double>& variances, const std::vector<double>& weights, double p) {
    double result = 0.0;
    int num_components = means.size();

    for (int i = 0; i < num_components; ++i) {
        double F_x = gaussian_cdf(x, means[i], variances[i]);
        result += weights[i] * (p - x) * F_x;
    }

    return result;
}


double find_maximum_x(const std::vector<double>& means, const std::vector<double>& sigmas, const std::vector<double>& weights, double p,
                      double learning_rate = 30, int max_iterations = 1000, double tolerance = 1e-6) {
    double x = p / 6;
    double gradient = 0.0;

    for (int iter = 0; iter < max_iterations; ++iter) {
        // 计算梯度
        gradient = 0.0;
        int num_components = means.size();

        for (int i = 0; i < num_components; ++i) {
            double F_x = gaussian_cdf(x, means[i], sigmas[i]);
            double f_x = gaussian_pdf(x, means[i], sigmas[i]);
            gradient += weights[i] * (-1 * F_x + (p - x) * f_x);
        }

        // 更新 x
        x += learning_rate * gradient;

        // 检查是否收敛
        if (std::abs(gradient) < tolerance) {
            break;
        }
    }

    return x;
}

void expectation_step(const std::vector<double>& data, const std::vector<Gaussian>& gaussians,
                      std::vector<double>& weights, std::vector<double>& responsibilities) {
    int k = gaussians.size();
    int n = data.size();

    responsibilities.resize(k * n);

    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < k; ++j) {
            double pdf_value = gaussian_pdf(data[i], gaussians[j].mean, gaussians[j].variance);
            weights[j] = pdf_value;
            sum += pdf_value;
        }

        if (sum == 0.0) {
            double default_value = 1.0 / k;
            for (int j = 0; j < k; ++j) {
                weights[j] = default_value;
                responsibilities[i * k + j] = default_value / k;
            }
        } else {
            for (int j = 0; j < k; ++j) {
                weights[j] /= sum;
                responsibilities[i * k + j] = weights[j];
            }
        }
    }
}



void maximization_step(const std::vector<double>& data, const std::vector<double>& responsibilities,
                       std::vector<Gaussian>& gaussians) {
    int k = gaussians.size();
    int n = data.size();

    for (int j = 0; j < k; ++j) {
        double sum_weights = 0.0;
        double sum_weighted_values = 0.0;
        double sum_weighted_squares = 0.0;

        for (int i = 0; i < n; ++i) {
            double weight = responsibilities[i * k + j];
            sum_weights += weight;
            sum_weighted_values += weight * data[i];
            sum_weighted_squares += weight * std::pow(data[i], 2);
        }

        gaussians[j].mean = sum_weighted_values / sum_weights;
        gaussians[j].variance = (sum_weighted_squares / sum_weights) - std::pow(gaussians[j].mean, 2);
    }
}


void gaussian_mixture_model(const std::vector<double>& data, int k,
                            std::vector<Gaussian>& gaussians, std::vector<double>& weights) {
    int n = data.size();

    std::vector<double> responsibilities;

    initialize_random(gaussians, k);

    const int max_iterations = 50;
    int iteration = 0;
    double convergence_threshold = 1e-6;
    double prev_log_likelihood = 0.0;

    while (iteration < max_iterations) {
        expectation_step(data, gaussians, weights, responsibilities);
        maximization_step(data, responsibilities, gaussians);

        double log_likelihood = 0.0;
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < k; ++j) {
                weights[j] = 0.1;
                sum += weights[j] * gaussian_pdf(data[i], gaussians[j].mean, gaussians[j].variance);
            }
            log_likelihood += std::log(sum);
        }

        if (std::abs(log_likelihood - prev_log_likelihood) < convergence_threshold) {
            break;
        }

        prev_log_likelihood = log_likelihood;
        iteration++;
    }
}


int gaussian_test(std::vector<double> data, int k) {
    std::vector<Gaussian> gaussians(k);
    std::vector<double> weights(k);
    gaussian_mixture_model(data, k, gaussians, weights);

    // Print the estimated Gaussian components
    for (int i = 0; i < k; ++i) {
        std::cout << "Gaussian " << i + 1 <<
        ": Mean = " << gaussians[i].mean <<
        ", Variance = " << gaussians[i].variance <<
        ", Weight = " << std::fixed << std::setprecision(4) << weights[i] << std::endl;
    }

    return 0;
}


class GMM {
public:
    void Init(const std::vector<double> &inputData, const int clustNum = 10, double eps = 0.01, double max_steps = 20);
    void train();
    int predicate(double x);
    void print();
    std::vector<double> means;
    std::vector<double> sigmas;
    std::vector<double> probilities;

protected:
    int clusterNum;
    std::vector<double> means_bkp;
    std::vector<double> sigmas_bkp;
    std::vector<double> probilities_bkp;
    std::vector<std::vector<double>> memberships;
    std::vector<std::vector<double>> memberships_bkp;
    std::vector<double> data;
    int dataNum;
    double epslon;
    double max_steps;

private:
    double gauss(const double x, const double m, const double sigma);
};

void GMM::Init(const vector<double> &inputData, const int clustNum, double eps, double max_steps)
{

    this->data = inputData;
    this->dataNum = data.size();

    this->clusterNum = clustNum;
    this->epslon = eps;
    this->max_steps = max_steps;

    this->means.resize(clusterNum);
    this->means_bkp.resize(clusterNum);
    this->sigmas.resize(clusterNum);
    this->sigmas_bkp.resize(clusterNum);

    this->memberships.resize(clusterNum);
    this->memberships_bkp.resize(clusterNum);
    for (int i = 0; i < clusterNum; i++)
    {
        memberships[i].resize(data.size());
        memberships_bkp[i].resize(data.size());
    }

    this->probilities.resize(clusterNum);
    this->probilities_bkp.resize(clusterNum);

    for (int i = 0; i < clusterNum; i++)
    {
        probilities[i] = probilities_bkp[i] = 1.0 / (double)clusterNum;

        means[i] = means_bkp[i] = 255.0*i / (clusterNum);

        sigmas[i] = sigmas_bkp[i] = 50;
    }
}

void GMM::train()
{
    //compute membership probabilities
    int i, j, k, m;
    double sum = 0, sum2;
    int steps = 0;
    bool go_on;
    do
    {
        for (k = 0; k < clusterNum; k++)
        {


            for (j = 0; j < data.size(); j++)
            {

                sum = 0;
                for (m = 0; m < clusterNum; m++)
                {
                    sum += probilities[m] * gauss(data[j], means[m], sigmas[m]);
                }

                memberships[k][j] = probilities[k] * gauss(data[j], means[k], sigmas[k]) / sum;
            }

            sum = 0;
            for (i = 0; i < dataNum; i++)
            {
                sum += memberships[k][i];
            }
            sum2 = 0;
            for (j = 0; j < dataNum; j++)
            {
                sum2 += memberships[k][j] * data[j];
            }

            means[k] = sum2 / sum;

            sum2 = 0;
            for (j = 0; j < dataNum; j++)
            {
                sum2 += memberships[k][j] * (data[j] - means[k])*(data[j] - means[k]);
            }
            sigmas[k] = sqrt(sum2 / sum);

            probilities[k] = sum / dataNum;
        }
        go_on = false;
        for (k = 0; k<clusterNum; k++)
        {
            if (means[k] - means_bkp[k]>epslon)
            {
                go_on = true;
                break;
            }
        }

        this->means_bkp = means;
        this->sigmas_bkp = sigmas;
        this->probilities_bkp = probilities;
    } while (go_on&&steps++ < max_steps);

    data.resize(1);
}

double GMM::gauss(const double x, const double m, const double sigma)
{
    return 1.0 / (sqrt(2 * 3.1415926)*sigma)*exp(-0.5*(x - m)*(x - m) / (sigma*sigma));
}


int GMM::predicate(double x)
{
    double max_p = -100;
    int i;
    double current_p;
    int bestIdx = 0;
    for (i = 0; i < clusterNum; i++)
    {
        current_p = gauss(x, means[i], sigmas[i]);
        if (current_p > max_p)
        {
            max_p = current_p;
            bestIdx = i;
        }
    }
    return bestIdx;
}

void GMM::print()
{
    int i;
    for (i = 0; i < clusterNum; i++)
    {
        std::cout << "Gaussian " << i + 1 <<
        ": Mean = " << means[i] <<
        ", Variance = " << sigmas[i] <<
        ", Weight = " << std::fixed << std::setprecision(4) << probilities[i] << std::endl;
    }
}



#endif //TEST_GAUSSIAN_H
