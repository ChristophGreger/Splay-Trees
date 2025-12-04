#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

#include "JobQueue.h"

static inline std::string trim(const std::string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

void runInteractiveDemo(JobQueue &queue)
{
    std::cout << "JobQueue Interactive Demo\n";
    std::cout << "Commands:\n";
    std::cout << "  add <priority> <VRT> <name>   - Insert a job\n";
    std::cout << "  remove <name>                 - Remove a job\n";
    std::cout << "  process                       - Process next job\n";
    std::cout << "  show                          - Pretty-print tree\n";
    std::cout << "  quit                          - Exit demo\n\n";

    std::string line;
    while (true)
    {
        std::cout << "> " << std::flush;

        if (!std::getline(std::cin, line))
        {
            std::cout << "\nExiting interactive demo.\n";
            break;
        }

        line = trim(line);
        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "quit")
        {
            std::cout << "Exiting interactive demo.\n";
            break;
        }
        else if (cmd == "add")
        {
            unsigned int priority = 0, vrt = 0;
            std::string name;

            if (!(iss >> priority >> vrt))
            {
                std::cout << "Usage: add <priority> <VRT> <name>\n";
                continue;
            }

            std::getline(iss, name);
            name = trim(name);
            if (name.empty())
            {
                std::cout << "Usage: add <priority> <VRT> <name>\n";
                continue;
            }

            JobData jd = {priority, vrt,
                          std::chrono::steady_clock::now(), name};

            queue.insert(jd);
        }
        else if (cmd == "process")
        {
            if (!queue.jobsAvailable())
            {
                std::cout << "No jobs to process.\n";
                continue;
            }

            JobData job = queue.processNextJob();
        }
        else if (cmd == "show")
        {
            queue.printTree();
        }
        else if (cmd == "remove")
        {
            std::string name;
            if (!(iss >> name))
            {
                std::cout << "Usage: remove <name>\n";
                continue;
            }

            queue.removeJobByName(name);
        }
    }
}

TEST(InteractiveDemo, InteractiveJobQueueCLI)
{
    JobQueue queue(1);
    runInteractiveDemo(queue);
    SUCCEED() << "Interactive demo ended.";
}
