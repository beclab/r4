#include <iostream>
#include <string>
#include <cstdlib>
#include "dump_traceinfo.h"
#include "knowledgebase_api.h"

// Function declaration
void printHelp();

int main(int argc, char *argv[])
{
    // Check the number of arguments
    if (argc < 2)
    {
        std::cerr << "Error: Missing arguments." << std::endl;
        printHelp();
        return 1;
    }

    std::string source;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--source")
        {
            if (i + 1 < argc)
            {
                source = argv[++i]; // Get the value of --source argument
            }
            else
            {
                std::cerr << "Error: --source requires a value." << std::endl;
                return 1;
            }
        }
        else if (arg == "--help" || arg == "-h")
        {
            printHelp();
            return 0;
        }
        else
        {
            std::cerr << "Error: Unknown argument '" << arg << "'." << std::endl;
            printHelp();
            return 1;
        }
    }

    // If --source argument is specified
    if (!source.empty())
    {
        knowledgebase::init_global_terminus_recommend_params();
        knowledgebase::EntryCache::getInstance().init();
        dump_traceinfo_main(source);
    }
    else
    {
        std::cerr << "Error: --source parameter is required." << std::endl;
        printHelp();
        return 1;
    }

    return 0;
}

void printHelp()
{
    std::cout << "Usage: tool [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --source recommend_source   Specify the source to dump" << std::endl;
    std::cout << "  --help, -h        Show this help message." << std::endl;
}
