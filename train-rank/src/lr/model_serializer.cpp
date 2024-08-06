#include "model_serializer.h"
#include "logistic_regression.h"
#include "../easylogging++.h"

#include <algorithm>
#include <fstream>
#include <cpprest/json.h>

using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

namespace lr {

bool SimpleModelSerializer::loadModel(const string& filename, LogisticRegression** model) {
    try {
        std::ifstream f(filename);
        std::stringstream str;

        str << f.rdbuf();
        f.close();

        auto json = web::json::value::parse(str);

        if (!json.is_object()) {
            LOG(ERROR) << "Failed to load model: invalid json" << endl;
            return false;
        }
        if (!json.has_field("weights")) {
            LOG(ERROR) << "Failed to load model: no weights in json" << endl;
            return false;
        }
        auto weights_json = json["weights"];
        if (!weights_json.is_array()) {
            LOG(ERROR) << "Failed to load model: invalid weights in json" << endl;
            return false;
        }

        vector<double> weights;
        for (auto item : weights_json.as_array()) {
            if (!item.is_number()) {
                LOG(ERROR) << "Failed to load model: invalid weights in json" << endl;
                return false;
            }
            weights.push_back(item.as_number().to_double());
        }

        return new LogisticRegression(weights.size() - 1, weights);
    } catch (web::json::json_exception e) {
        LOG(ERROR) << "Failed to load model, exception: " << e.what() << endl;
        return false;
    }

    return true;
}

bool SimpleModelSerializer::saveModel(const std::string& filename, const LogisticRegression& model) {
    try {
        std::vector<web::json::value> weights;
        std::transform(model.weights.begin(), model.weights.end(), std::back_inserter(weights),
            [](const auto v) { return web::json::value(v); } );
        web::json::value json;
        json["weights"] = web::json::value::array(weights);

        ofstream os(filename);
        os << json.serialize().c_str();
        os.close();
    } catch (web::json::json_exception e) {
        LOG(ERROR) << "Failed to save model, exception: " << e.what() << endl;
        return false;
    }
    return true;
}

}  // namespace lr