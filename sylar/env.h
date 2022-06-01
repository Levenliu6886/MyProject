/**
 * 环境变量
 * getenv
 * setenv
 * 
 * /proc/pid/cmdline|cwd|exe
 * 
 * 查看进程 ps aux | grep xxx
 * ls -lh /proc/进程号/
 * 就可以看到进程内的数据
 * 
 * 利用/proc/pid/cmdline
 * 在全局变量构造函数， 实现在进入main函数前解析参数
 * 
 * 1.读写环境变量
 * 2.获取程序的绝对路径， 基于绝对路径设置cwd
 * 3.可以通过cmdline， 在进入main函数之前，解析好参数
 */

#ifndef __SYLAR_ENV_H__
#define __SYLAR_ENV_H__

#include "singleton.h"
#include "thread.h"
#include <map>
#include <vector>

namespace sylar {

class Env {
public:
    typedef RWMutex RWMutexType;
    bool init(int argc, char** argv);

    void add(const std::string& key, const std::string& val);
    bool has(const std::string& key);
    void del(const std::string& key);
    std::string get(const std::string& key, const std::string& default_val = "");

    void addHelp(const std::string& key, const std::string& desc);
    void removeHelp(const std::string& key);
    void printHelp();

    const std::string& getExe() const { return m_exe; }
    const std::string& getCwd() const { return m_cwd; }

    bool setEnv(const std::string& key, const std::string& val);
    std::string getEnv(const std::string& key, const std::string& default_value = "");

    std::string getAbsolutePath(const std::string& path) const;
private:
    RWMutexType m_mutex;
    std::map<std::string, std::string> m_args;
    std::vector<std::pair<std::string, std::string> > m_helps;//参数，参数的意义

    std::string m_program;//当前的程序
    std::string m_exe;//可执行文件的绝对路径
    std::string m_cwd;
};

typedef sylar::Singleton<Env> EnvMgr;

}

#endif