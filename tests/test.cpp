//
// Created by Christoph Greger on 07.11.25.
//

#include "JobQueue.h"
#include <gtest/gtest.h>
#include <chrono>

using std::chrono::steady_clock;
using std::chrono::milliseconds;

TEST(JobQueueTest, ConstructorRejectsZeroN) {
    EXPECT_THROW(JobQueue q(0), std::invalid_argument);
}

TEST(JobQueueTest, ProcessNextJobOnEmptyQueue) {
    JobQueue q(1);
    EXPECT_THROW(q.processNextJob(), std::runtime_error);
}

TEST(JobQueueTest, JobsAvailableAfterInsert) {
    JobQueue q(5);
    auto now = steady_clock::now();
    JobData job{1, 10, now, "job1"};

    EXPECT_FALSE(q.jobsAvailable());
    q.insert(job);
    EXPECT_TRUE(q.jobsAvailable());
}

TEST(JobQueueTest, PriorityOrdering) {
    JobQueue q(100);

    auto now = steady_clock::now();
    JobData low{1, 5, now, "low"};
    JobData mid{2, 5, now, "mid"};
    JobData high{3, 5, now, "high"};

    q.insert(mid);
    q.insert(low);
    q.insert(high);

    EXPECT_EQ(q.processNextJob().jobName, "high");
    EXPECT_EQ(q.processNextJob().jobName, "mid");
    EXPECT_EQ(q.processNextJob().jobName, "low");
    EXPECT_FALSE(q.jobsAvailable());
}

TEST(JobQueueTest, VrtOrderingForEqualPriority) {
    JobQueue q(100);

    auto now = steady_clock::now();
    JobData longer{1, 10, now, "long"};
    JobData shorter{1, 5, now, "short"};

    q.insert(longer);
    q.insert(shorter);

    EXPECT_EQ(q.processNextJob().jobName, "short");
    EXPECT_EQ(q.processNextJob().jobName, "long");
}

TEST(JobQueueTest, TimestampTieBreak) {
    JobQueue q(100);

    auto base = steady_clock::now();
    JobData older{1, 5, base - milliseconds(10), "old"};
    JobData newer{1, 5, base, "new"};

    q.insert(newer);
    q.insert(older);

    EXPECT_EQ(q.processNextJob().jobName, "old");
    EXPECT_EQ(q.processNextJob().jobName, "new");
}

TEST(JobQueueTest, TimeSlicingAndReinsertion) {
    JobQueue q(3); // time slice = 3 seconds

    auto base = steady_clock::now();
    JobData j1{1, 5, base, "j1"};
    JobData j2{1, 4, base - milliseconds(1), "j2"};

    q.insert(j1);
    q.insert(j2);

    JobData r1 = q.processNextJob(); // j2 first
    EXPECT_EQ(r1.jobName, "j2");
    EXPECT_EQ(r1.VRT, 1);

    JobData r2 = q.processNextJob(); // j2 finishes
    EXPECT_EQ(r2.jobName, "j2");
    EXPECT_EQ(r2.VRT, 1);

    JobData r3 = q.processNextJob(); // j1 reduced
    EXPECT_EQ(r3.jobName, "j1");
    EXPECT_EQ(r3.VRT, 2);

    JobData r4 = q.processNextJob(); // j1 finishes
    EXPECT_EQ(r4.jobName, "j1");
    EXPECT_EQ(r4.VRT, 2);

    EXPECT_FALSE(q.jobsAvailable());
}