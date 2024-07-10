#pragma once

#include <memory>
#include <string>
#include <vector>

namespace lr {

class LogisticRegression {
public:
    LogisticRegression(int dim);
    // Only used by builder
    LogisticRegression(int dim, const std::vector<double>& weights);

public:
    double cost(const std::vector<std::vector<double> >& pos, const std::vector<std::vector<double> >& neg);
    double activation(const std::vector<double>& x);
    void fit(const std::vector<std::vector<double> >& pos,
            const std::vector<std::vector<double> >& neg,
            double learningRate,
            int epoch);
    double predict(const std::vector<double>& features);

    std::vector<double> weights;  // b at last
private:
    void gradientDecsent(const std::vector<double>& x, double y, double rate);

    int dimension;
};

}