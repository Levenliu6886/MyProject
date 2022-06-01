#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
//线程里获取协程调度器的指针
static thread_local Scheduler* t_scheduler = nullptr;
//线程里的主协程
static thread_local Fiber* t_fiber = nullptr;

//把自己执行了，把自己放到协程调度里面去了
//它只是创建了一个协程，这个协程执行他的run方法，这个协程还没有真正被执行起来
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name){
    SYLAR_ASSERT(threads > 0);

    if(use_caller) {
        sylar::Fiber::GetThis();
        --threads;

        SYLAR_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        //因为它使用了use_caller，所以这个main协程不能参与到我们这个run方法中
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //防止这个线程名字没有改过来
        sylar::Thread::SetName(m_name);

        //在一个线程里面声明一个调度器， 再把当前线程放到调度器里面，它的主协程不再是线程的主协程
        //而是在执行run方法的主协程
        t_fiber = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    }else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

//获取当前的协程调度器
Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_fiber;
}

//启动线程池
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }

    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    // if(m_rootFiber) {
    //     //m_rootFiber->swapIn();
    //     m_rootFiber->call();
    //     SYLAR_LOG_INFO(g_logger) << "call out" << m_rootFiber->getState();
    // }
}
void Scheduler::stop() {
    m_autoStop = true;
    //use_caller并且只有一个线程的情况
    if(m_rootFiber && m_threadCount == 0 
            && ((m_rootFiber->getState() == Fiber::TERM) 
            || (m_rootFiber->getState() == Fiber::INIT))) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        //留给子类去实现
        if(stopping()) {
            return;
        }
    }
    //当前线程退出
    //bool exit_on_this_fiber = false;
    if(m_rootThread != -1) {
        SYLAR_ASSERT(GetThis() == this);
        //当这个scheduler要把他创建时的线程也使用上去的时候，它的stop一定要在创建的线程上执行

    }else {
        //能在任意线程上执行stop
        SYLAR_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        //有多少个线程就唤醒多少次，唤醒之后它们就会自己结束自己
        tickle();
    }

    if(m_rootFiber) {
        //唤醒
        tickle();
    }

    if(m_rootFiber) {
        // while(!stopping()) {
        //     if(m_rootFiber->getState() == Fiber::TERM 
        //         || m_rootFiber->getState() == Fiber::EXCEPT) {
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //         SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
        //         t_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }
        if(!stopping()) {
            m_rootFiber->call();
        }
        
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }

    // if(stopping()) {
    //     return;
    // }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

//新创建的线程直接在run里面跑
void Scheduler::run() {
    SYLAR_LOG_INFO(g_logger) << "run";
    set_hook_enable(true);
    //return;
    //先把当前的schedule置成自己
    setThis();
    //如果线程id不等于主线程id
    if(sylar::GetThreadId() != m_rootThread) {
        //run起来的线程，把这个线程=主线程的fiber
        t_fiber = Fiber::GetThis().get();
    }

    //当任务都调度完之后去做idle
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        //协程消息队列里面取出消息
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //如果一个任务已经指定好了在哪个线程下面执行，当前正在执行这个run的会不等于这个
                //线程的话，我们就不处理它
                if(it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;//发出信号需要通知别人来处理
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {//正在执行
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }

        if(tickle_me) {
            tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            }else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD; // 因为它让出了执行时间，它的状态就该变成hold状态                
            }
            ft.reset();
        }else if(ft.cb) {
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {//说明协程状态已经执行完成
                cb_fiber->reset(nullptr);//智能指针的reset，不会引起析构
            } else {//if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount; 
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }

}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        sylar::Fiber::YeidTohold();
    }
}


}