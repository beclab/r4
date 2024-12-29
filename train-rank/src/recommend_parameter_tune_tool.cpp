#include "knowledgebase_api.h"

void print_usage()
{
    std::cout << "Usage: \n";
    std::cout << "\t--" << TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION << " <value> (default 10)\n";

    std::cout << "\t--" << TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION << " <value> (default 10000)\n";

    std::cout << "\t--" << TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE << " <value> (default 0.3)\n";

    std::cout << "\t--" << TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE << "<value> (default 0.5)\n";

    std::cout << "\t--" << TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD << " <value> (default 10)\n";

    std::cout << "\t--" << TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING << " <long|short> (default long)\n";

    std::cout << "\t--" << TERMINUS_RECOMMEND_TRACE_INFO_NUMBER << " <value> (default 100)\n";
}

std::function<void(const std::string &)> get_short_term_user_embedding_number_of_impression_lambda()
{
    auto short_term_user_embedding_number_of_impression_lambda = [](const std::string &value)
    {
        try
        {
            int number = std::stoi(value);
            if (number <= 0)
            {
                LOG(ERROR) << "short_term_user_embedding_number_of_impression_lambda value should be greater than 0" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << "short_term_user_embedding_number_of_impression_lambda value should be integer " << e.what() << std::endl;
        }
        web::json::value current_value;
        current_value["value"] = web::json::value::string(value);
        knowledgebase::updateKnowledgeConfig(TERMINUS_RECOMMEND_PARAMETER,
                                             std::string(TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION),
                                             current_value);
    };
    return short_term_user_embedding_number_of_impression_lambda;
}

std::function<void(const std::string &)> get_long_term_user_embedding_number_of_impression_lambda()
{
    auto long_term_user_embedding_number_of_impression_lambda = [](const std::string &value)
    {
        try
        {
            int number = std::stoi(value);
            if (number <= 0)
            {
                LOG(ERROR) << "long_term_user_embedding_number_of_impression_lambda value should be greater than 0" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << "long_term_user_embedding_number_of_impression_lambda value should be integer " << e.what() << std::endl;
        }
        web::json::value current_value;
        current_value["value"] = web::json::value::string(value);
        knowledgebase::updateKnowledgeConfig(TERMINUS_RECOMMEND_PARAMETER,
                                             std::string(TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION),
                                             current_value);
    };
    return long_term_user_embedding_number_of_impression_lambda;
}

std::function<void(const std::string &)> get_long_term_user_embedding_weight_for_rankscore_lambda()
{
    auto long_term_user_embedding_weight_for_rankscore_lambda = [](const std::string &value)
    {
        try
        {
            double number = std::stod(value);
            if (number < 0 || number > 1)
            {
                LOG(ERROR) << "long_term_user_embedding_weight_for_rankscore value should be between 0 and 1" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << "long_term_user_embedding_weight_for_rankscore value should be float " << e.what() << std::endl;
        }
        web::json::value current_value;
        current_value["value"] = web::json::value::string(value);
        knowledgebase::updateKnowledgeConfig(TERMINUS_RECOMMEND_PARAMETER,
                                             std::string(TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE),
                                             current_value);
    };
    return long_term_user_embedding_weight_for_rankscore_lambda;
}

std::function<void(const std::string &)> get_article_time_weight_for_rankscore_lambda()
{
    auto article_time_weight_for_rankscore_lambda = [](const std::string &value)
    {
        try
        {
            double number = std::stod(value);
            if (number < 0 || number > 1)
            {
                LOG(ERROR) << "article_time_weight_for_rankscore value should be between 0 and 1" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << "article_time_weight_for_rankscore value should be float " << e.what() << std::endl;
        }
        web::json::value current_value;
        current_value["value"] = web::json::value::string(value);
        knowledgebase::updateKnowledgeConfig(TERMINUS_RECOMMEND_PARAMETER,
                                             std::string(TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE),
                                             current_value);
    };
    return article_time_weight_for_rankscore_lambda;
}

std::function<void(const std::string &)> get_cold_start_article_clicked_number_threshold_lambda()
{
    auto cold_start_article_clicked_number_threshold_lambda = [](const std::string &value)
    {
        try
        {
            int number = std::stoi(value);
            if (number <= 0)
            {
                LOG(ERROR) << "cold_start_article_clicked_number_threshold value should be greater than 0" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << "cold_start_article_clicked_number_threshold value should be integer " << e.what() << std::endl;
        }
        web::json::value current_value;
        current_value["value"] = web::json::value::string(value);
        knowledgebase::updateKnowledgeConfig(TERMINUS_RECOMMEND_PARAMETER,
                                             std::string(TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD),
                                             current_value);
    };
    return cold_start_article_clicked_number_threshold_lambda;
}

std::function<void(const std::string &)> get_long_or_short_embedding_as_recall_embedding_lambda()
{
    auto long_or_short_embedding_as_recall_embedding_lambda = [](const std::string &value)
    {
        if (value != "long" && value != "short")
        {
            LOG(ERROR) << "long_or_short_embedding_as_recall_embedding value should be long or short" << std::endl;
        }
        web::json::value current_value;
        current_value["value"] = web::json::value::string(value);
        knowledgebase::updateKnowledgeConfig(TERMINUS_RECOMMEND_PARAMETER,
                                             std::string(TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING),
                                             current_value);
    };
    return long_or_short_embedding_as_recall_embedding_lambda;
}

std::function<void(const std::string &)> get_trace_info_number_lambda()
{
    auto trace_info_number_lambda = [](const std::string &value)
    {
        try
        {
            int number = std::stoi(value);
            if (number <= 0)
            {
                LOG(ERROR) << "trace_info_number value should be greater than 0" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << "trace_info_number value should be integer " << e.what() << std::endl;
        }
        web::json::value current_value;
        current_value["value"] = web::json::value::string(value);
        knowledgebase::updateKnowledgeConfig(TERMINUS_RECOMMEND_PARAMETER,
                                             std::string(TERMINUS_RECOMMEND_TRACE_INFO_NUMBER),
                                             current_value);
    };
    return trace_info_number_lambda;
}

bool parse_command_line(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage();
        return false;
    }

    std::unordered_map<std::string, std::function<void(const std::string &)>>
        param_map = {
            {"--" + std::string(TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION), get_short_term_user_embedding_number_of_impression_lambda()},
            {"--" + std::string(TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION), get_long_term_user_embedding_number_of_impression_lambda()},
            {"--" + std::string(TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE), get_long_term_user_embedding_weight_for_rankscore_lambda()},
            {"--" + std::string(TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE), get_article_time_weight_for_rankscore_lambda()},
            {"--" + std::string(TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD), get_cold_start_article_clicked_number_threshold_lambda()},
            {"--" + std::string(TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING), get_long_or_short_embedding_as_recall_embedding_lambda()},
            {"--" + std::string(TERMINUS_RECOMMEND_TRACE_INFO_NUMBER), get_trace_info_number_lambda()}};

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (param_map.find(arg) != param_map.end() && i + 1 < argc)
        {
            param_map[arg](argv[++i]);
        }
        else
        {
            // std::cerr << "Error: Unknown argument " << arg << std::endl;
            LOG(ERROR) << "Error: Unknown argument " << arg << std::endl;
            // print_usage();
            continue;
        }
    }

    return true;
}

void print_current_parameter()
{
    std::cout << TERMINUS_RECOMMEND_SHORT_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION << "  " << globalTerminusRecommendParams.short_term_user_embedding_impression_count << std::endl;
    std::cout << TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_NUMBER_OF_IMPRESSION << "  " << globalTerminusRecommendParams.long_term_user_embedding_impression_count << std::endl;
    std::cout << TERMINUS_RECOMMEND_LONG_TERM_USER_EMBEDDING_WEIGHT_FOR_RANKSCORE << "  " << globalTerminusRecommendParams.long_term_user_embedding_weight_for_rankscore << std::endl;
    std::cout << TERMINUS_RECOMMEND_ARTICLE_TIME_WEIGHT_FOR_RANKSCORE << "  " << globalTerminusRecommendParams.article_time_weight_for_rankscore << std::endl;
    std::cout << TERMINUS_RECOMMEND_COLD_START_ARTICLE_CLICKED_NUMBER_THRESHOLD << "  " << globalTerminusRecommendParams.cold_start_article_clicked_number_threshold << std::endl;
    std::cout << TERMINUS_RECOMMEND_LONG_OR_SHORT_EMBEDDING_AS_RECALL_EMBEDDING << globalTerminusRecommendParams.long_or_short_embedding_as_recall_embedding << std::endl;
    std::cout << TERMINUS_RECOMMEND_TRACE_INFO_NUMBER << "  " << globalTerminusRecommendParams.trace_info_number_zip << std::endl;
}

int main(int argc, char *argv[])
{
    if (!parse_command_line(argc, argv))
    {
        return 1;
    }
    knowledgebase::init_global_terminus_recommend_params();
    print_current_parameter();
}