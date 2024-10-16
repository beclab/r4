#include "logistic_regression.h"
#include "../easylogging++.h"

#include <cmath>
#include <fstream>

using std::string;
using std::vector;
using std::log;
using std::exp;

namespace lr {
namespace {

double sigmoid(double x) {
    return 1.0 / (exp(-x) + 1.0);
}

}  // namespace

LogisticRegression::LogisticRegression(int dim): dimension(dim) {
    weights.resize(dim + 1);

    // Random weights
    for (int i = 0; i < dim; ++i) {
        weights[i] = (rand() % 1000 - 500) / 10000.0;
    }

    weights.back() = 0.0;
}

LogisticRegression::LogisticRegression(int dim, const vector<double>& weights): dimension(dim), weights(weights) {
}

double LogisticRegression::cost(const vector<vector<double> >& pos, const vector<vector<double> >& neg) {
    double result = 0.0;
    for (auto& x : pos) {
        auto z = activation(x);
        result += -log(z);
    }

    for (auto& x : neg) {
        auto z = activation(x);
        result += (1 - z) * log(z);
    }

    return result;
}

double LogisticRegression::activation(const vector<double>& x) {
    double z = weights.back();
    for (int i = 0; i < dimension; ++i) {
        z += weights[i] * x[i];
    }

    return sigmoid(z);
}

void LogisticRegression::gradientDecsent(const vector<double>& x, double y, double rate) {
    for (int i = 0; i < dimension; ++i) {
        weights[i] -= rate * (y - activation(x)) * x[i];
    }
    weights.back() -= rate * (y - activation(x));
}

void LogisticRegression::fit(const std::vector<std::vector<double> >& pos,
        const std::vector<std::vector<double> >& neg, double learningRate, int epoch) {
    for (int i = 0; i < epoch; ++i) {
        for (const auto& sample : pos) {
            gradientDecsent(sample, 1.0, learningRate / pos.size());
        }
        for (const auto& sample : neg) {
            gradientDecsent(sample, 0.0, learningRate / neg.size());
        }
    }
}

double LogisticRegression::predict(const vector<double>& x) {
    return activation(x);
}

}  // namespace lr

/*
#include <iostream>

int main() {  // For testing
    lr::LogisticRegression lr(5, 0.01);

    vector<vector<double>> pos, neg;
    pos.push_back(vector<double>{0.5, 0.5, 1.0, 1.0, 0.5});
    pos.push_back(vector<double>{0.5, 0.1, 1.0, 1.0, 0.5});
    pos.push_back(vector<double>{0.2, 5, 1.0, 1.0, 0.5});
    neg.push_back(vector<double>{0.5, 0.3, 1.0, 1, 0.1});
    neg.push_back(vector<double>{0.2, 0.5, 5.0, 2.0, 0.1});
    neg.push_back(vector<double>{0.2, 0.3, 1.0, 1.5, 0.1});

    for (int i = 0; i < 2000; ++i) {
        for (int i = 0; i < pos.size(); ++i) {
            lr.gradientDecsent(pos[i], 1.0);
        }

        for (int i = 0; i < neg.size(); ++i) {
            lr.gradientDecsent(neg[i], 0.0);
        }

        if (i % 50 == 0) {
            std::cout << "i = " << i << ", cost = " << lr.cost(pos, neg) << std::endl;
        }
    }

    for (int i = 0; i < 6; ++i) {
        std::cout << lr.weights[i] << ", ";
    }
    std::cout << std::endl;
}*/