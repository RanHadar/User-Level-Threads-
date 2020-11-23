//******************************************INCLUDES***********************************************
#include <iostream>
#include "Scheduler.h"
//*************************************************************************************************

#define EXIT_FAILURE 1
#define SUCCESS 0
#define FAILURE -1

using namespace std;

//*************************************************************************************************
Scheduler::Scheduler(): readyThreadsList(),runningThreadId(0)
{
    numOfThreads = 1;
}

Scheduler::~Scheduler()  = default;

int Scheduler::addNewThread(int threadID)
{
    readyThreadsList.push_back(threadID);
    numOfThreads++;
    return SUCCESS;
}

int Scheduler::addToReady(int threadID){
    readyThreadsList.push_back(threadID);
}
/**
 * Switch threads when the Quantum time is over
 * @return running thread.
 */
int Scheduler::quantumChange() {

    readyThreadsList.push_back(runningThreadId);
    runningThreadId = readyThreadsList.front();
    readyThreadsList.pop_front();
    return runningThreadId;
}

void Scheduler::popFrontReady(){
    runningThreadId = readyThreadsList.front();
    readyThreadsList.pop_front();

}


bool Scheduler::checkMaxNumThread(){
    return (numOfThreads == MAX_THREADS);
}

bool Scheduler::checkReadyThreads() {
    return not readyThreadsList.empty();
}

void Scheduler::deleteFromReady(int tid){
    readyThreadsList.remove(tid);
}

void Scheduler::terminateThread(int tid){
    readyThreadsList.remove(tid);

}

void Scheduler::decreaseTotalThreads(){
    numOfThreads--;
}


void Scheduler::printReadyList()
{
    std::cout << "readyList:";
    for(auto id: readyThreadsList)
    {
        std::cout << id;
    }
    std::cout<< "\n";
}

int Scheduler::getRunningId(){
    return runningThreadId;
}





