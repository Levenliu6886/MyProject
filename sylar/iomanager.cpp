#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <error.h>
#include <string.h>
#include<iostream>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    SYLAR_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name) 
    :Scheduler(threads, use_caller, name) {
    //epoll_create函数创建的资源与套接字相同，也有操作系统管理
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);
    // epoll 方式通过如下结构体 epoll_event 将发生变化的文件描述符单独集中在一起
    //struct epoll_event {
    //    __uint32_t events;
    //    epoll_data_t data;
    //}
    //typedef union epoll_data {
    //  void *ptr;
    //  int fd;
    //  __uint32_t u32;
    //  __uint64_t u64;
    //} epoll_data_t;

    //声明足够大的epoll_event结构体数组后，传递给epoll_wait函数时，发生变化的文件描述符信息将被填入该数组
    //因此，该函数和创建套接字的情况相同，也会返回文件描述符，也就是说返回的文件描述符主要用于区分 epoll 例程。
    //需要终止时，与其他文件描述符相同，也要调用 close 函数
    
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));

    //边缘触发可以分离数据和处理数据的时间点
    //边缘触发中输入缓冲收到数据时仅注册 1 次该事件。即使输入缓冲中还留有数据，也不会再进行注册
    event.events = EPOLLIN | EPOLLET;// 可读状态 边缘触发（只会触发一次，不处理则不会再输出）
    event.data.fd = m_tickleFds[0];

    //fcntl修改一些具体的属性
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);//设置文件状态标记 非阻塞模式
    SYLAR_ASSERT(!rt);
    //- EPOLLIN：需要读取数据的情况
    //- EPOLLOUT：输出缓冲为空，可以立即发送数据的情况
    //- EPOLLPRI：收到 OOB 数据的情况
    //- EPOLLRDHUP：断开连接或半关闭的情况，这在边缘触发方式下非常有用
    //- EPOLLERR：发生错误的情况
    //- EPOLLET：以边缘触发的方式得到事件通知
    //- EPOLLONESHOT：发生一次事件后，相应文件描述符不再收到事件通知。因此需要向 epoll_ctl 函数的第二个参数传递 EPOLL_CTL_MOD ，再次设置事件。
    //添加事件  （epoll句柄，添加事件，哪个句柄上面加，加哪个事件）
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

    //m_fdContexts.resize(32);
    contextResize(64);

    //创建好之后默认启动scheduler的start
    start();
}

//将里面的句柄关掉
IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

//0 success -1 error
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) { //表明空间足够
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events & event) {
        //一般情况一个句柄不可能两次加同一个事件，这意味这两个线程在同时操作一个句柄的同一个方法
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd = "
                        << fd << " event=" << event
                        << " fd_ctx.event=" << fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;//操作类型
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;//原来的events加上现在的event就是新的events
    epevent.data.ptr = fd_ctx;//回调的时候通过数据字段拿回是在哪个fdcontext上面触发的

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);//这样就把事件加到epoll里面去了
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
            //strerror用于打印错误码对应的字符串描述
            
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    //有值的话说明上面已经有事件了
    SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;

}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);//~event:把event去掉
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ","<< fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" <<strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}
//事件有一定的条件要触发，把事件取消并强制触发
bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);//~event:把event去掉
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ","<< fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" <<strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;  
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events)) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ","<< fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" <<strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
    
}
IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if(!hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout ==~0ull && m_pendingEventCount == 0
                && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();//会执行默认构造函数，执行值的初始化
    //智能指针并不支持数组，但是可以用析构的方法
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            SYLAR_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
            break;
        }
        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;// 5s
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            /*
                成功时返回发生事件的文件描述符个数，失败时返回 -1
                epfd : 表示事件发生监视范围的 epoll 例程的文件描述符
                events : 保存发生事件的文件描述符集合的结构体地址值
                maxevents : 第二个参数中可以保存的最大事件数
                timeout : 以 1/1000 秒为单位的等待时间，传递 -1 时，一直等待直到发生事件
            */
            rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);

            //表示操作系统在数据回来前强制中断了，应该尝试下一次
            if(rt < 0 && errno == EINTR) {

            } else {
                break;
            }
        }while(true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                while(read(m_tickleFds[0], &dummy, 1) == 1);
                continue;

            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)) {//错误或者中断
                event.events |= EPOLLIN | EPOLLOUT;//更改成读和写
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {//如果是EPOLLIN的话 "&" : =
                real_events |= READ;//那么是读事件
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            //如果它没事件,则什么也不用做
            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            //剩余事件（当前事件减掉上面已经触发了的事件）
            int left_event = (fd_ctx->events & ~real_events);
            int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_event;//  "|" : +

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << "," << fd_ctx->fd << "," << event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        //处理完idle之后，应该把idle控制权让出来，让到它协程调度框架里面去
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

//唤醒epollwait让它重新设置定时时间
void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}