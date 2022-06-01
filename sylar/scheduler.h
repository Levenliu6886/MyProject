/**
 *           1-N         1-M
 * scheduler ---> thread ---> fiber
 * 1、线程池， 分配一组线程
 * 2、线程调度器，将协程指定到相应的线程上去执行
 * 
 * N ： M
 */

#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"
#include<iostream>
namespace sylar {

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType; 

    //在use_caller相当于复用了一个已存在的协程，但是已存在的协程它的主协程跑的不是run方法
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    //获取当前的协程调度器
    static Scheduler* GetThis();
    static Fiber* GetMainFiber();//返回当前协程调度器的调度协程

    void start();
    void stop();

    //在协程调度器里执行一个协程
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if(need_tickle) {
            tickle();
        }

    }
    //批量
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            //批量的优点就是锁一次，锁一次把所有的放进去，确保这一组连续的任务一定是在连续的消息队列里
            //它们一定会按照连续的顺序去执行
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
        
    }
    
protected:
    virtual void tickle();//通知协程调度器有任务了
    void run();
    virtual bool stopping();
    virtual void idle();//没任务做执行idle，当执行到idle时，表面到了空闲状态
    void setThis();
    bool hasIdleThreads() { return m_idleThreadCount > 0;}
private:
    //返回true表明，以前是没有任务的，可能所有的线程都陷入到内核态来，wait到某个信号量
    //当放进去一个，就需要唤醒一个线程，通知有任务了
    //协程调度启动
    template<class Fiber0rCb>
    bool scheduleNoLock(Fiber0rCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    struct FiberAndThread
    {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {

        }

        FiberAndThread(Fiber::ptr* f, int thr) 
            :thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), thread(thr) {
            
        }

        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
            cb.swap(*f);
        }

        //当将其放到stl中时候，它在给其分配对象的时候一定需要默认构造函数
        FiberAndThread() 
            :thread(-1) {

        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }

    };
    

private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    //里面放的是即将要执行的或者计划要执行的协程
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_rootFiber;//use_caller为true时有效, 调度协程
    std::string m_name;

protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};//活跃线程数量
    std::atomic<size_t> m_idleThreadCount = {0};//空闲线程数量
    bool m_stopping = true;//是否停止
    bool m_autoStop = false;//是否主动停止
    int m_rootThread = 0;//主线程id（usecaller id）
};

}

#endif 