#ifndef __SYLAR_DAEMON_H__
#define __SYLAR_DAEMON_H__

#include <functional>
#include <unistd.h>
#include <stdint.h>
#include "singleton.h"

namespace sylar {

struct ProcessInfo {
    pid_t parent_id = 0;//父进程id
    pid_t main_id = 0;//主进程id
    uint64_t parent_start_time = 0;//父进程启动时间
    uint64_t main_start_time = 0;//主进程启动时间
    uint32_t restart_count = 0;//主进程重启次数
    std::string toString() const;
};

typedef sylar::Singleton<ProcessInfo> ProcessInfoMgr;

    //启动程序，可以选择用守护进程的方式
int start_daemon(int argc, char** argv
                    , std::function<int(int argc, char** argv)> main_cb
                    , bool is_daemon);//bool is_daemon是否守护进程的方式
}


#endif