//
// Created by Christoph Greger on 19.11.25.
//

#include "JobQueue.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <algorithm>

using std::chrono::steady_clock;

void Giver(JobQueue *queue)
{

    // Give some jobs to the queue. Also wait sometimes to simulate real world conditions.
    JobData jobd1 = {1, 6, steady_clock::now(), "Job1"};
    JobData jobd2 = {100, 1, steady_clock::now(), "Job2"};
    JobData jobd3 = {5, 2, steady_clock::now(), "Job3"};
    JobData jobd4 = {3, 3, steady_clock::now(), "Job4"};
    JobData jobd5 = {1, 10, steady_clock::now(), "Job5"};
    JobData jobd6 = {2, 1, steady_clock::now(), "Job6"};

    queue->insert(jobd1);
    queue->insert(jobd2);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    queue->insert(jobd3);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    queue->insert(jobd4);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    queue->insert(jobd5);
    queue->insert(jobd6);
}

void Worker(JobQueue *queue)
{
    while (!queue->stop)
    {
        if (queue->jobsAvailable())
        {
            JobData job = queue->processNextJob();
            auto timetorun = std::min(queue->N, job.VRT);
            std::this_thread::sleep_for(std::chrono::seconds(timetorun));
        }
        else
        {
            std::cout << "No job available" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    std::cout << "Worker stopped.\n";
}

TEST(Demo, GiverandWorker)
{
    JobQueue queue(1);
    std::thread giverThread(Giver, &queue);
    std::thread workerThread(Worker, &queue);

    // Let the worker run for a while
    std::this_thread::sleep_for(std::chrono::seconds(24));

    giverThread.join();

    queue.stop = true;
    workerThread.join();
}