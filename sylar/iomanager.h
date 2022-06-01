/**
 * IOManager(epoll) --->Scheduler
 *      |
 *      |
 *   idle(epoll_wait)
 * 
 *    信号量
 *  PutMessage(msg) +信号量1,single()
 *  message_queue
 *      |
 *      |------Thread
 *      |------Thread
 *          wait() -信号量1，RecvMessage(msg,)
 * 
 * 异步IO，等待数据返回。epoll_wait,其被唤醒要么数据返回，要么消息队列有任务了
 * 
 * epoll_create, epoll_ctl(修改/删除事件)，epoll_wait
 */

#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__ 

#include "scheduler.h"
#include "timer.h"

namespace sylar {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE  = 0x0,
        READ  = 0x1, //EPOLLIN
        WRITE = 0x4, //EPOLLOUT
    };
private:
    //因为在epoll_wait加事件的话，是针对一个fd的，它所拥有的事件，就放到fd里面去
    struct  FdContext {
        typedef Mutex MutexType;
        //每种事件有相应的实例
        struct EventContext {
            Scheduler* scheduler = nullptr;    //事件执行的scheduler
            Fiber::ptr fiber;        //事件协程
            std::function<void()> cb;//事件的回调函数
        };

        //根据事件类型获取事件的那个类
        EventContext& getContext(Event event);
        
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        EventContext read;      //读事件
        EventContext write;     //写事件
        int fd = 0;             //事件关联的句柄

        Event events = NONE;  //已经注册的事件
        MutexType mutex;
    };
public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();
    
    //1 success, 0 retry, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    //找到了对应的事件并强制触发执行
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);
private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};//现在等待执行的事件数量
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

    
}

#endif 