#pragma once

#include <string>

#include "logistic_regression.h"

namespace lr {

class ModelSerializer {
public:
    virtual bool loadModel(const std::string&, LogisticRegression**) = 0;
    virtual bool saveModel(const std::string&, const LogisticRegression&) = 0;
};

class SimpleModelSerializer : public ModelSerializer {
public:
    virtual bool loadModel(const std::string&, LogisticRegression**);
    virtual bool saveModel(const std::string&, const LogisticRegression&);
};

}