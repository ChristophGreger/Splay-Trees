//
// Created by Christoph Greger on 07.11.25.
//

#include "JobQueue.h"
#include <stdexcept>
#include <iostream>
#include <utility>
#include <vector>

bool JobQueue::less(const JobData& jobData1, const JobData& jobData2) {
    //The "biggest" job is the next job.
    return (jobData1.priority < jobData2.priority)
        || (jobData1.priority == jobData2.priority) && (jobData1.VRT > jobData2.VRT)
        || ((jobData1.priority == jobData2.priority) && (jobData1.VRT == jobData2.VRT) && (jobData1.timestamp> jobData2.timestamp));
}

Job::Job(JobData jobData, Job* parent, Job* left, Job* right)
    : left(left), right(right), parent(parent), jobData(std::move(jobData)) {}

JobQueue::JobQueue(unsigned int N) : root(nullptr), N(N) {
    if (N == 0) {
        throw std::invalid_argument("Time slice N must be > 0.");
    }
}

JobQueue::~JobQueue() {

    // Iterative destructor. As depth can be O(n), recursion may lead to stack overflow.
    if (!root) return;
    std::vector<Job*> st;
    Job* curr = root;
    Job* last = nullptr;
    while (curr || !st.empty()) {
        if (curr) {
            st.push_back(curr);
            curr = curr->left;
        } else {
            Job* node = st.back();
            if (node->right && last != node->right) {
                //If right subtree not yet processed
                curr = node->right;
            } else {
                // If no right subtree or already processed
                st.pop_back();
                last = node;
                delete node;
            }
        }
    }
    root = nullptr;
}

Job* JobQueue::subtreeMin(Job* n) {
    if (!n) {
        return nullptr;
    }
    while (n->left) {
        n = n->left;
    }
    return n;
}

Job* JobQueue::subtreeMax(Job* n) {
    if (!n) {
        return nullptr;
    }
    while (n->right) {
        n = n->right;
    }
    return n;
}

void JobQueue::rotateLeft(Job* x) {
    Job* y = x->right;
    if (!y) {
        return;
    }

    x->right = y->left;
    if (y->left) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (!x->parent) {
        root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

void JobQueue::rotateRight(Job* x) {
    Job* y = x->left;
    if (!y) {
        return;
    }

    x->left = y->right;
    if (y->right) {
        y->right->parent = x;
    }

    y->parent = x->parent;
    if (!x->parent) {
        root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->right = x;
    x->parent = y;
}

void JobQueue::zig(Job* x) {
    if (!x->parent) {
        return;
    }
    if (x->parent->left == x) {
        rotateRight(x->parent);
    } else {
        rotateLeft(x->parent);
    }
}

void JobQueue::zigzig(Job* x) {
    Job* p = x->parent;
    Job* g = p->parent;
    if (!g) {
        return;
    }
    if (p->left == x && g->left == p) {
        rotateRight(g);
        rotateRight(p);
    } else if (p->right == x && g->right == p) {
        rotateLeft(g);
        rotateLeft(p);
    }
}

void JobQueue::zigzag(Job* x) {
    Job* p = x->parent;
    Job* g = p->parent;
    if (!g) {
        return;
    }

    if (p->left == x && g->right == p) {
        rotateRight(p);
        rotateLeft(g);
    } else if (p->right == x && g->left == p) {
        rotateLeft(p);
        rotateRight(g);
    }
}

void JobQueue::splay(Job* x) {
    if (!x) {
        return;
    }

    while (x->parent) {
        if (!x->parent->parent) {
            zig(x);
        } else if ((x->parent->left == x && x->parent->parent->left == x->parent) ||
                   (x->parent->right == x && x->parent->parent->right == x->parent)) {
            zigzig(x);
        } else {
            zigzag(x);
        }
    }
}

void JobQueue::insert(const JobData &jobData) {
    std::lock_guard<std::mutex> lock(mtx);
    insertNonBlocking(jobData);
    std::cout << "Inserted job: " << jobData.jobName << " with priority " << jobData.priority << " and VRT " << jobData.VRT << std::endl;
}

void JobQueue::insertNonBlocking(const JobData &jobData) {

    if (!root) {
        root = new Job(jobData);
        return;
    }

    Job* x = root;
    Job* p = nullptr;

    while (x) {
        p = x;
        if (JobQueue::less(jobData, x->jobData)) {
            x = x->left;
        } else if (JobQueue::less(x->jobData, jobData)) {
            x = x->right;
        } else {
            //Someone tried to insert the same job twice. We do not allow this. (and it actually shouldn't be possible
            std::cerr << "Someone tried to insert a Job twice. What a bad idea! Only differing by name doesn't work!" << std::endl;
            return;
        }
    }

    Job* n = new Job(jobData, p);
    if (JobQueue::less(jobData, p->jobData)) {
        p->left = n;
    } else {
        p->right = n;
    }
    splay(n);
}

void JobQueue::removeJob(Job* t) {
    if (!t) {
        return;
    }

    splay(t);

    Job* L = t->left;
    Job* R = t->right;

    if (L) {
        L->parent = nullptr;
    }
    if (R) {
        R->parent = nullptr;
    }

    delete t;
    root = nullptr;

    if (!L) {
        root = R;
        return;
    }

    Job* m = subtreeMax(L);
    splay(m);
    m->right = R;

    if (R) {
        R->parent = m;
    }

    root = m;
}

bool JobQueue::jobsAvailable() const {
    std::lock_guard<std::mutex> lock(mtx);
    return root != nullptr;
}

JobData JobQueue::processNextJobNonBlocking() {
    if (!root) {
        throw std::runtime_error("No jobs in the queue.");
    }

    Job* nextJob = subtreeMax(root);
    JobData jobData = nextJob->jobData;

    removeJob(nextJob);

    if (jobData.VRT > N) {
        JobData returning = jobData;
        jobData.VRT -= N;
        insertNonBlocking(jobData);
        std::cout << jobData.jobName << " is now being processed for " << N << " time units. Remaining VRT afterwards:" << jobData.VRT << std::endl;
        return returning;
    }
    std::cout << jobData.jobName << " is now being processed for " << jobData.VRT << " time units. Job is finished afterwards." << std::endl;
    return jobData;
}

JobData JobQueue::processNextJob() {
    std::lock_guard<std::mutex> lock(mtx);
    return processNextJobNonBlocking();
}
