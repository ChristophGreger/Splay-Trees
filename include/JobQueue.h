//
// Created by Christoph Greger on 07.11.25.
//

#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include <chrono>
#include <string>
#include <mutex>
#include <atomic>

using std::string;
using std::chrono::steady_clock;
using std::chrono::time_point;

struct JobData
{
    const unsigned int priority;
    unsigned int VRT; // in seconds, therefore int is sufficient.
    const time_point<steady_clock> timestamp;
    const string jobName;
};

struct Job
{
    Job *left;
    Job *right;
    Job *parent;
    JobData jobData;
    explicit Job(JobData jobData, Job *parent = nullptr, Job *left = nullptr, Job *right = nullptr);
};

class JobQueue
{

private:
    mutable std::mutex mtx;

    Job *root;

    static bool less(const JobData &jobData1, const JobData &jobData2);

    static Job *subtreeMin(Job *n); // leftmost in subtree
    static Job *subtreeMax(Job *n); // rightmost in subtree

    void rotateLeft(Job *x);
    void rotateRight(Job *x);
    void zig(Job *x);
    void zigzig(Job *x);
    void zigzag(Job *x);
    void splay(Job *x);
    void removeJob(Job *x);

    void insertNonBlocking(const JobData &jobData);
    JobData processNextJobNonBlocking();

public:
    explicit JobQueue(unsigned int N);
    ~JobQueue();
    const unsigned int N;

    void insert(const JobData &jobData);
    std::atomic<bool> stop{false};
    [[nodiscard]] bool jobsAvailable() const; // Returns true if there are jobs in the queue.
    JobData processNextJob();                 // Returns the JobData of the next job and removes it from the queue or reduces its VRT.
};

#endif // JOB_QUEUE_H