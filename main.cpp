#include "coroutine.h"

#include <iostream>

using namespace std;

CoroutineScheduler* sched = NULL;

uintptr_t func1(void* args)
{
    uintptr_t ret;
    cout << "function1 now, args:" << static_cast<const char*>(args) << ", start to yield." << endl;
    
    ret = sched->Yield(reinterpret_cast<uintptr_t>("func1 yield 1"));
    cout << "func1.1 return from yield:" << (const char*)ret << endl;
    
    ret = sched->Yield(reinterpret_cast<uintptr_t>("func1 yield 2"));
    cout << "func1.2 return from yield:" << reinterpret_cast<const char*>(ret) << endl;
    
    return reinterpret_cast<uintptr_t>("func1 stop");
}

uintptr_t func2(void* args)
{
    cout << "function2 now, args:" << static_cast<const char*>(args) << ", start to yield." << endl;

    uintptr_t y = sched->Yield((uintptr_t)("func2 yield 1"));
    cout << "func2 return from yield:" << reinterpret_cast<const char*>(y) << ", going to stop" << endl;

    return reinterpret_cast<uintptr_t>("func2 stop");
}

int main(int argc, char** argv)
{
    CoroutineScheduler s;
    sched = &s;

    bool stop = false;
    int f1 = s.CreateCoroutine(func1, (void*)"start func1");
    int f2 = s.CreateCoroutine(func2, (void*)"start func2");

    while (!stop)
    {
        stop = true;

        if (s.IsAlive(f1))
        {
            stop = false;
            const char* y1 = (const char*)s.Resume(f1, (uintptr_t)"resume func1");
            cout << "func1 yield:" << y1 << endl;
        }

        if (s.IsAlive(f2))
        {
            stop = false;
            const char* y2 = (const char*)s.Resume(f2, (uintptr_t)"resume func2");
            cout << "func2 yield:" << y2 << endl;
        }
    }

    return 0;
}