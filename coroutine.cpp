#include <map>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>
#include <iostream>
#include <stdio.h>

#include "coroutine.h"

using std::cout;
using std::endl;
using std::map;

struct coroutine
{
    void* arg;
    
    int status;
    ucontext_t cxt;
    uintptr_t yield;
    CoroutineScheduler::func f;
    char stack[0];
};

class CoroutineScheduler::SchedulerImpl
{
public:
    SchedulerImpl(int stacksize);

    ~SchedulerImpl();

    int CreateCoroutine(CoroutineScheduler::func f, void* arg);

    int DestoryCoroutine(int id);

    bool IsAlive(int id) const;

    uintptr_t Yield(uintptr_t p);

    uintptr_t Resume(int id, uintptr_t p);

private:
    SchedulerImpl(const SchedulerImpl&);

    SchedulerImpl& operator=(const SchedulerImpl&);

    static void Schedule(void* arg);

private:
    int m_index;
    int m_running;
    int m_stacksize;

    ucontext_t m_mainContext;
    map<int, coroutine*> m_id2coroutine;
};

CoroutineScheduler::CoroutineScheduler(int size) 
: m_impl(new SchedulerImpl(size))
{
}

CoroutineScheduler::~CoroutineScheduler()
{
    free(m_impl);
}

int CoroutineScheduler::CreateCoroutine(func f, void* args)
{
    return m_impl->CreateCoroutine(f, args);
}

int CoroutineScheduler::DestoryCoroutine(int id)
{
    return m_impl->DestoryCoroutine(id);
}

uintptr_t CoroutineScheduler::Resume(int id, uintptr_t p /*= NULL*/)
{
    return m_impl->Resume(id, p);
}

bool CoroutineScheduler::IsAlive(int id) const
{
    return m_impl->IsAlive(id);
}

uintptr_t CoroutineScheduler::Yield(uintptr_t p)
{
    return m_impl->Yield(p);
}

CoroutineScheduler::SchedulerImpl::SchedulerImpl(int stacksize)
: m_index(0), m_running(false), m_stacksize(stacksize)
{
}

CoroutineScheduler::SchedulerImpl::~SchedulerImpl()
{
    for (auto iter = m_id2coroutine.begin(); iter != m_id2coroutine.end(); ++iter)
    {
        if (iter->second)
        {
            free(iter->second);
        }
    }
}

bool CoroutineScheduler::SchedulerImpl::IsAlive(int id) const
{
    auto it = m_id2coroutine.find(id);
    if (it == m_id2coroutine.end())
    {
        return false;
    }
    return it->second;
}


int CoroutineScheduler::SchedulerImpl::CreateCoroutine(CoroutineScheduler::func f, void* arg)
{
    coroutine* cor = (coroutine*)malloc(sizeof(coroutine) + m_stacksize);

    if (!cor)
    {
        return -1;
    }

    cor->arg = arg;
    cor->f = f;
    cor->yield = 0;
    cor->status = READY;

    int index = m_index++;
    m_id2coroutine[index] = cor;

    return index;
}

int CoroutineScheduler::SchedulerImpl::DestoryCoroutine(int id)
{
    auto iter = m_id2coroutine.find(id);
    if (iter == m_id2coroutine.end())
    {
        return -1;
    }
    coroutine* cor = iter->second;
    free(cor);
    m_id2coroutine.erase(id);
    return id;
}

void CoroutineScheduler::SchedulerImpl::Schedule(void* arg)
{
    assert(arg);

    SchedulerImpl* s = static_cast<SchedulerImpl*>(arg);

    int id = s->m_running;
    if (s->m_id2coroutine.find(id) == s->m_id2coroutine.end())
    {
        cout << "failed to find id " << id << endl;
        return;
    }

    coroutine* cor = s->m_id2coroutine[id];
    assert(cor);

    cor->yield = cor->f(cor->arg);

    s->m_running = -1;
    cor->status = FINISHED;
}

uintptr_t CoroutineScheduler::SchedulerImpl::Resume(int id, uintptr_t p)
{
    coroutine* cor = m_id2coroutine[id];

    if (cor == NULL || cor->status == RUNNING)
    {
        return 0;
    }

    cor->yield = p;
    switch (cor->status)
    {
    case READY:
        {
            getcontext(&cor->cxt);
            
            cor->status = RUNNING;
            cor->cxt.uc_stack.ss_sp = cor->stack;
            cor->cxt.uc_stack.ss_size = m_stacksize;
            cor->cxt.uc_link = &m_mainContext;

            m_running = id;
            makecontext(&cor->cxt, (void(*)())Schedule, 1, this);
            swapcontext(&m_mainContext, &cor->cxt);
        }
        break;
    case SUSPEND:
        {
            m_running = id;
            cor->status = RUNNING;
            swapcontext(&m_mainContext, &cor->cxt);
        }
        break;
    default:
        assert(0);
    }

    uintptr_t ret = cor->yield;
    cor->yield = 0;

    if (m_running == -1 && cor->status == FINISHED)
    {
        DestoryCoroutine(id);
    }

    return ret;
}

uintptr_t CoroutineScheduler::SchedulerImpl::Yield(uintptr_t p)
{
    if (m_running < 0)
    {
        return 0;
    }

    int cur = m_running;
    m_running = -1;
    
    coroutine* cor = m_id2coroutine[cur];
    cor->yield = p;
    cor->status = SUSPEND;

    swapcontext(&cor->cxt, &m_mainContext);

    uintptr_t ret = cor->yield;
    cor->yield = 0;
    return ret;
}

