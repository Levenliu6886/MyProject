#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar{

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count){
    if(sem_init(&m_semaphore, 0, count)) {//0表示信号量将被进程内的线程共享
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    //函数sem_wait( sem_t *sem )被用来阻塞当前线程直到信号量sem的值大于0，
    //解除阻塞后将sem的值减一，表明公共资源经使用后减少。
    //函数sem_trywait ( sem_t *sem )是函数sem_wait（）的非阻塞版本，它直接将信号量sem的值减一
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    //函数sem_post( sem_t *sem )用来增加信号量的值。当有线程阻塞在这个信号量上时，
    //调用这个函数会使其中的一个线程不在阻塞，选择机制同样是由线程的调度策略决定的
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}

Thread* Thread::GetThis(){
    return t_thread;
}

const std::string& Thread::GetName(){
    return t_thread_name;
}

void Thread::SetName(const std::string& name){
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb),m_name(name){
        if(name.empty()) {
            m_name = "UNKNOW";
        }
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
                    << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
        m_semaphore.wait();
}
Thread::~Thread(){
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join(){
    //pthread_join()主线程等待子线程的终止。
    //也就是在子线程调用了pthread_join()方法后面的代码，只有等到子线程结束了才能执行。
    if(m_thread) {
        //等待m_thread退出
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void* Thread::run(void* arg){
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();
    //pthread_setname_np给线程命名，修改了名称之后在拓扑里面也会看到对应的名称，但是名称最大字符数是16
    //pthread_self返回的是posix定义的线程ID,用来区分某个进程中不同的线程
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    //防止当这个函数里面有智能指针的时候，防止其引用会一直不被释放，这样swap之后至少少了个引用
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();

    cb();
    return 0;
}

}