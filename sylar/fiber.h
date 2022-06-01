#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__ 

#include <memory>
#include <ucontext.h>
#include "thread.h"

namespace sylar {

class Scheduler;
//继承这个类可以获取当前类的智能指针，不能在这个栈上创建对象，因为它一定要是智能指针的成员
//它的构造函数会把里面的智能指针关联起来
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,//正在执行
        TERM,//结束
        READY,
        EXCEPT
    };
private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();
    
    //当协程执行完了函数也执行完了，结束状态了，但是内存分配了，可以利用其栈上内存继续另外一个协程，省了内存分配和释放
    //重置协程函数，并重置状态 //INIT,TERM
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();

    //强行把当前协程置换成目标执行协程
    void call();

     /**
     * @brief 将当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     */
    void back();

    uint64_t getId() const { return m_id; }

    State getState() const { return m_state; }
public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //协程切换到后台，并且设置为ready状态
    static void YeidToReady();
    //协程切换到后台，并且设置为hold状态
    static void YeidTohold();
    //总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;//栈的内存空间

    std::function<void()> m_cb;
};

}

#endif 