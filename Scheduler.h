#ifndef EX2_SCHEDULER_H
#define EX2_SCHEDULER_H

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <deque>
#include <list>
#include "Thread.h"

#define MAX_THREADS 100

class Scheduler {

public:

    explicit Scheduler();

    ~Scheduler();

    int addNewThread(int threadID);

    int quantumChange();

    int getRunningId();

    bool checkMaxNumThread();

    bool checkReadyThreads();

    void popFrontReady();

    void deleteFromReady(int tid);
    
    void printReadyList();

    int addToReady(int threadID);

    void terminateThread(int tid);

    void decreaseTotalThreads();

private:

//    std::deque<int> blockedThreadsList;
    std::list<int> readyThreadsList;
    Thread* runThread;
    int numOfThreads;
    int runningThreadId;
};


#endif //EX2_SCHEDULER_H
