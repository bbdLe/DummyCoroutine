#ifndef __COROUTINE_H
#define __COROUTINE_H
#include <stdint.h>
#include <cstdlib>

class CoroutineScheduler
{
public:
    enum Status
    {
        READY,
        SUSPEND,
        RUNNING,
        FINISHED
    };
    
    typedef uintptr_t(*func)(void* arg);

public:
    CoroutineScheduler(int size = 1024 * 1000);

    ~CoroutineScheduler();

    int CreateCoroutine(func f, void* args);

    int DestoryCoroutine(int id);

    uintptr_t Yield(uintptr_t p = 0);

    uintptr_t Resume(int id, uintptr_t p = 0);

    bool IsAlive(int id) const;

private:
    // Can't impl
    CoroutineScheduler(const CoroutineScheduler&);

    CoroutineScheduler& operator=(const CoroutineScheduler&);

private:
    class SchedulerImpl;
    SchedulerImpl* m_impl;
};

#endif