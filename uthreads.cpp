//******************************************Includes*******************************************************
#include <iostream>
#include "uthreads.h"
#include "Scheduler.h"
#include "Thread.h"
#include <vector>
#include <queue>
//*********************************************************************************************************
#define SUCCESS 0
#define FAILURE -1

using namespace std;
//*******************************************Variables*****************************************************
priority_queue<int, std::vector<int>, greater<int>> freeIDs; //priority queue for free IDs
static Scheduler scheduler;
Thread *threads[MAX_THREADS];
unsigned int quantumDuration;
struct sigaction sigAction;
struct itimerval timer;
sigset_t alarmSignal;
int *qunatum_time;
int totalQuantums;
int sizeQ;

//*************************************Function Declarations*************************************************
void generateAllThreadsID();
void initAllthreads();
int getTimeforPriority(int priority);
void cleanAndExit(int exitcode);
int setTimer(int quantum_usecs);
int checkValidPriority(int priority);
void initSignals();
//************************************************************************************************************


/**
 * Blocking the alarm signal in critical sections.
 */
void blockAlarmSignal()
{ //todo - if statement if this masking failed - then exit&clean
    sigprocmask(SIG_BLOCK, &alarmSignal, &threads[scheduler.getRunningId()]->mask);
}

/**
 * Set the mask to the running-thread's regular mask.
 */
void unmaskSignals()
{
    sigprocmask(SIG_SETMASK, &threads[scheduler.getRunningId()]->mask, nullptr);
}


void switchThreads(){

    int runningThread = scheduler.quantumChange();
    threads[runningThread]->incQuantumCounter();
    threads[runningThread]->setState(RUN);
    setTimer(getTimeforPriority(threads[runningThread]->getPriority()));
    siglongjmp(*threads[scheduler.getRunningId()]->getEnv(),5);
}


/**
 * Handling with the timer signals - switching to next thread
 * @param sig
 */
void timer_handler(int sig) // Switch threads
{
    blockAlarmSignal(); //todo - check is valid
    totalQuantums ++;

    if (scheduler.checkReadyThreads())//in case there is a thread in ready list
    {
        int runningTid = scheduler.getRunningId();
        int ret_val = sigsetjmp(threads[runningTid]->env, 1);
        if (ret_val == 5) {
            //if the return value of sigsetjump != 0, save the current signal mask as well
            unmaskSignals();
            return;
        }
        //switch to the next thread in the ready list
        int temp = scheduler.getRunningId();
        threads[temp]->setState(READY);
        switchThreads();

    }
    threads[scheduler.getRunningId()]->incQuantumCounter();
    //todo - check if readylist is empty, thread 0 should continue running
    unmaskSignals();
}

/**
 * Sets the timer.
 * @return 0 on success, -1 otherwise.
 */
int setTimer(int quantum_usecs)
{

    timer ={0};
    int sec = quantum_usecs/1000000;
    int micro_sec= quantum_usecs%1000000;
    //first timing
    timer.it_value.tv_sec = sec;
    timer.it_value.tv_usec = micro_sec;
    //intervals
    timer.it_interval.tv_sec = sec;
    timer.it_interval.tv_usec = micro_sec;

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)) {
        cerr <<  "system call error: setitimer error.\n";
        cleanAndExit(EXIT_FAILURE); // todo - check which exit code
    }
    return SUCCESS;
}


int uthread_init(int *quantum_usecs, int size)
{
    if(size <= 0)
    {
        cerr << "thread library error: Wrong input - invalid size input\n";
        return FAILURE;
    }
    for(int i =0 ;i < size ;i++)
    {
        if(quantum_usecs[i] <= 0)
        {
            cerr << "thread library error: Wrong input - invalid qunatum duration input\n";
            return FAILURE;
        }
    }
    qunatum_time = quantum_usecs;
    sizeQ = size;
    generateAllThreadsID();
    //Initializing all threads
    initAllthreads();
    //Setting the Main thread
    threads[0] = new Thread(0, nullptr, STACK_SIZE, 0);
    threads[0]->setState(RUN);
    threads[0]->incQuantumCounter();
    totalQuantums = 1;
    //Setting the timer
    quantumDuration = getTimeforPriority(0);
    initSignals();
    setTimer(quantumDuration);

    return SUCCESS;
}

void initSignals(){

    if(sigemptyset(&alarmSignal) == -1){
        std::cerr <<  "system error: sigemptyset error\n";
        exit(1);
    }

    if(sigaddset(&alarmSignal, SIGVTALRM) == -1){
        cerr << "system error: Error adding signal.\n";
        cleanAndExit(1);
    } //adding alarm signal to the signal set

    sigAction.sa_handler = &timer_handler; //sets a new handler for the alarm signal
    if (sigaction(SIGVTALRM, &sigAction, nullptr) < 0) {
        cerr << "system error: Sigaction Error.\n";
        cleanAndExit(EXIT_FAILURE); // todo - check with which exit
    }

}

bool handle_signals(int state){
    if(sigprocmask(state, &alarmSignal, NULL) == -1){
        std::cerr << "system error: failed at blocking signals\n";
        return false;
    }
    return true;
}

/**
 * Genereates all threads IDs
 */
void generateAllThreadsID() {
    for(int i = 1; i < MAX_THREADS; i++){
        freeIDs.push(i);
    }
}

/**
 * Initializing all the threads
 */
void initAllthreads(){
    for(auto &thread: threads)
    {
        thread = nullptr;
    }
}
/**
 * Checks if the thread's ID is valid
 * @param threadID
 * @return
 */
int checkID(int threadID){
    if(threadID < 0 || threadID > 99){
        cerr << "thread library error: invalid thread id input\n";
        return FAILURE;
    }
    else if (threads[threadID] == nullptr) {
        cerr << "thread library error: no thread with id = "<< threadID << "\n";
        return FAILURE;
    }
    return SUCCESS;
}


int generateTid()
{
    int nextFreeId = freeIDs.top();
    freeIDs.pop();
    return nextFreeId;
}


int uthread_spawn(void (*f)(void), int priority)
{
    //checks if the priority input is valid.
    if(checkValidPriority(priority)==FAILURE){
        return FAILURE;
    }
    blockAlarmSignal(); //blocking the alaram signals for the current running thread.
    //Check if number of threads reached the max number
    if(scheduler.checkMaxNumThread()){

        cerr << "thread library error: reached max number of threads\n";
        return FAILURE;
    }
    int tid = generateTid();
    // add new thread to threads list objects
    threads[tid] = new Thread(tid, f, STACK_SIZE, priority);
    scheduler.addNewThread(tid); //push to ready list, managed by the scheduler
    unmaskSignals();
    return tid;
}

/**
 * Gets the quantum time per priority
 * @param priority
 * @return priority's quantum time
 */
int getTimeforPriority(int priority)
{
    return qunatum_time[priority];
}


void cleanAndExit(int exitcode) {
    for(auto thread:threads)
    {
        delete(thread);
    }
    qunatum_time = nullptr;
    exit(exitcode);
}


int uthread_change_priority(int tid, int priority){
    blockAlarmSignal();
    if(checkID(tid) == FAILURE || checkValidPriority(priority)==FAILURE){
        return FAILURE;
    }
    if(tid == scheduler.getRunningId()){
        threads[tid]->setNewPriority(priority);
    }else {
        threads[tid]->setPriority(priority);
    }
    if(!handle_signals(SIG_UNBLOCK)){return -1;}
    return SUCCESS;
}

int uthread_get_tid(){
//    scheduler.printReadyList();
    return scheduler.getRunningId();
}

int uthread_get_total_quantums(){
    return totalQuantums;
}

int uthread_get_quantums(int tid){
    if(!handle_signals(SIG_BLOCK)){return -1;}
    if(checkID(tid)==FAILURE){
        return FAILURE;
    }
    if(!handle_signals(SIG_UNBLOCK)){return -1;}
    return threads[tid]->getQuantumCounter();
}

int uthread_terminate(int tid){
    blockAlarmSignal();
    if(checkID(tid)==FAILURE){
        return FAILURE;
    }
    if(tid == 0){ // case this is the main thread - terminate process
        cleanAndExit(0);
    }
    int state = threads[tid]->getState();
    delete(threads[tid]);
    threads[tid] = nullptr;
    freeIDs.push(tid);
    scheduler.decreaseTotalThreads();
    switch(state){
        case READY:
            scheduler.deleteFromReady(tid);
            break;

        case RUN:
            totalQuantums++;
            scheduler.popFrontReady();
            threads[scheduler.getRunningId()]->incQuantumCounter();
            threads[scheduler.getRunningId()]->setState(RUN);
            setTimer(getTimeforPriority(threads[scheduler.getRunningId()]->getPriority()));
            siglongjmp(*threads[scheduler.getRunningId()]->getEnv(),5);
            //switchThreads();
    }
    if(!handle_signals(SIG_UNBLOCK)){return -1;}
    return SUCCESS;
}

int uthread_block(int tid){
    blockAlarmSignal();
    if(checkID(tid) == FAILURE){
        return FAILURE;
    }
    if(tid == 0) // if we try to main thread
    {
        cerr << "thread library error: cannot block the main thread\n";
        return FAILURE;
    }
    Thread* bThread = threads[tid];
    int bThreadState =  threads[tid]->getState();
    if(bThreadState == RUN)
    {
        bThread->setState(BLOCKED);
        totalQuantums++;
        int ret_val = sigsetjmp(threads[tid]->env, 1);
        if (ret_val == 5) {
            unmaskSignals();
            return SUCCESS;
        }
        scheduler.popFrontReady();
        threads[scheduler.getRunningId()]->incQuantumCounter();
        threads[scheduler.getRunningId()]->setState(RUN);
        setTimer(getTimeforPriority(threads[scheduler.getRunningId()]->getPriority()));
        siglongjmp(*threads[scheduler.getRunningId()]->getEnv(),5);
    }
    else if(bThreadState ==READY) {
        bThread->setState(BLOCKED);
        scheduler.deleteFromReady(tid);
    }
    //todo - need to be aware that the thread is indeed in ready threads list
    //todo - for example initialize but not in ready list
    if(!handle_signals(SIG_UNBLOCK)){return -1;}
    return SUCCESS;
}

int uthread_resume(int tid){
    blockAlarmSignal();
    if(checkID(tid) == FAILURE){
        return FAILURE;
    }
    Thread* bThread = threads[tid];
    int bThreadsState = threads[tid]->getState();
    if(bThreadsState == BLOCKED){
        bThread->setState(READY);
        scheduler.addToReady(tid); //push to ready list, managed by the scheduler
    }
    if(!handle_signals(SIG_UNBLOCK)){return -1;}
    return SUCCESS;
}

int checkValidPriority(int priority){
    if(priority >= sizeQ || priority < 0){
        cerr << "thread library error: Invalid priority number\n";
        return FAILURE;
    }
    return SUCCESS;
}






