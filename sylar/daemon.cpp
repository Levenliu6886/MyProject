#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "config.h"
#include "daemon.h"
#include "log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static sylar::ConfigVar<uint32_t>::ptr g_daemon_restart_interval = 
    sylar::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::toString() const {
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id
       << " main_id=" << main_id
       << " parent_start_time=" << sylar::Time2Str(parent_start_time)
       << " main_start_time=" << sylar::Time2Str(main_start_time)
       << " restart_count=" << restart_count << "]";
    return ss.str();
}

static int real_start(int argc, char** argv
                    , std::function<int(int argc, char** argv)> main_cb) {
    return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv
                    , std::function<int(int argc, char** argv)> main_cb) {
    //第一个参数描述守护进程时要不要将执行的上下文路径指向根路径
    //第二个参数是要不要将标准输入输出流先关闭，默认情况下是要关闭的
    //deamon函数主要用于希望脱离控制台，以守护进程形式在后台运行的程序
    //deamon()之后调用了fork()，如果fork成功，那么父进程就主动调用_exit(2)退出，所以看到的错误信息 全部是子进程产生的。如果成功函数返回0，否则返回-1并设置errno
    daemon(1, 0);
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    while(true) {
        pid_t pid = fork();
        if(pid == 0) {
            ProcessInfoMgr::GetInstance()->main_id = getpid();
            ProcessInfoMgr::GetInstance()->main_start_time = time(0);
            SYLAR_LOG_INFO(g_logger) << "process start pid=" << getpid();
            return real_start(argc, argv, main_cb);
        } else if(pid < 0) {
            SYLAR_LOG_ERROR(g_logger) << "fork fail return=" << pid
                << " errno=" << errno << " errstr=" << strerror(errno);
            return -1;
        } else {
            int status = 0;
            //pid>0时，只等待进程ID等于pid的子进程，
            //不管其它已经有多少子进程运行结束退出了，只要指定的子进程还没有结束，waitpid就会一直等下去
            //status用来保存被收集进程退出时的一些状态，它是一个指向int类型的指针
            waitpid(pid, &status, 0);
            if(status) {
                SYLAR_LOG_ERROR(g_logger) << "child crash pid=" << pid
                    << " status=" << status;
            } else {
                SYLAR_LOG_INFO(g_logger) << "child finished pid=" << pid;
                break;
            }
            ProcessInfoMgr::GetInstance()->restart_count += 1;
            //为了让子进程资源释放干净
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv
                    , std::function<int(int argc, char** argv)> main_cb
                    , bool is_daemon) {
    if(!is_daemon) {
        return real_start(argc, argv, main_cb);
    } 
    return real_daemon(argc, argv, main_cb);
}

}