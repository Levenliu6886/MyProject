/**
 * Timer -> addTimer()-->cancel()
 * 获取当前的定时器触发离现在的时间差
 * 返回当前需要触发的定时器
 */

#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <set>
#include <vector>
#include "thread.h"

namespace sylar {
//不能自己创建，要由manager来创建
class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    bool cancel();
    bool refresh();//重设时间
    bool reset(uint64_t ms, bool from_now);
private:
    //因为需要manager来创建，所以把它设置成私有 
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
    Timer(uint64_t next);
    
private:
    bool m_recurring = false;//是否循环定时器
    uint64_t m_ms = 0;//执行周期
    uint64_t m_next = 0;//精确的执行时间
    std::function<void()> m_cb;
    TimerManager* m_manager = nullptr;
    
private:
    struct Comparator{
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
    
};

class TimerManager {
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
    //当它触发并且这个条件存在的时候才会触发，用智能指针做条件，因为智能指针有引用计数，如果这个智能指针消失了，就没有必要再执行了
    //比如加某个定时器，每过一定时间要清理某个智能指针上面的数据
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                            ,std::weak_ptr<void> weak_cond
                            ,bool recurring = false);
    uint64_t getNextTimer();//下一个执行时间点
    //已经触发了等待时间需要执行的回调函数
    void listExpiredCb(std::vector<std::function<void()>>& cbs);
    bool hasTimer();
protected:
    virtual void onTimerInsertedAtFront() = 0;
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
private:
    //检测是否调了时间，如果调了时间要做一些相应操作来适应过来
    bool detectClockRollover(uint64_t now_ms);
private:
    RWMutexType m_mutex;
    //因为定时器有时间顺序，所以选set
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    bool m_tickled = false;
    uint64_t m_previousTime = 0;//上一个执行的时间
};

}

#endif 