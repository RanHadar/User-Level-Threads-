#include "Thread.h"
#include <signal.h>
#include <iostream>
#include <vector>
#include <setjmp.h>

#define JB_SP 6
#define JB_PC 7

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t Thread::translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
            "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t Thread::translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}
#endif


Thread::Thread(int id, void (*f)(void),int stackSize, int priority): threadID(id)
                                    ,priority(priority),state(READY),new_priority(priority)
{
    //todo - understand if ready status is needed when initialzing a new thread.
    // set up for thread
    address_t sp, pc;
    sp = (address_t) stack + stackSize - sizeof(address_t);
    pc = (address_t) f;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);

}

Thread::~Thread() = default;


void Thread::setPriority(int new_priority){
    priority = new_priority;
}

void Thread::setNewPriority(int new_priority)
{
    priority = new_priority;
}

void Thread::setState(int new_state)
{
    state = new_state;
}

void Thread::checkPriority(){
    if(priority != new_priority){
        setPriority(new_priority);
    }
}