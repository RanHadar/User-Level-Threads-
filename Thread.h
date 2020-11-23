#ifndef EX2_THREAD_H
#define EX2_THREAD_H

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#define SECOND 1000000
#define STACK_SIZE 4096

#define RUN 0
#define READY 1
#define BLOCKED 2
#define INIT 3

typedef unsigned long address_t;

class Thread {

public:
    sigjmp_buf env;
    sigset_t mask;

    Thread(int id, void (*f)(void),int stackSize, int priority);

    ~Thread();

    int getThreadID(){
        return threadID;
    }

    int getPriority(){
        return priority;}

    int getState(){
        return state;
    }

    sigjmp_buf* getEnv(){
        return &env;}

    void incQuantumCounter(){
        quantumCounter++;
    }

    int getQuantumCounter(){ return quantumCounter; }

    void setState(int state);

    void setPriority(int new_priority);

    void setNewPriority(int new_priority);

    void checkPriority();

private:

    char stack[STACK_SIZE];
    int threadID; // thread's id
    int state; //thread's state
    unsigned int quantumCounter;
    int priority; //thread's priority
    int new_priority;
    static address_t translate_address(address_t addr); //static translate function
};

#endif //EX2_THREAD_H
