#include "util.h"
#include <execinfo.h> //backtrace
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>//access
#include <string.h>
#include <sys/stat.h>
#include "log.h"
#include "fiber.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId(){
    //getpid()得到的是进程的pid，在内核中，每个线程都有自己的pid，要得到线程的pid，必须用syscall(SYS_gettid)
    //pthread_self函数获取的是线程ID，线程ID在某进程中是唯一的，在不同的进程中创建的线程可能出现ID值相同的情况
    return syscall(SYS_gettid);
    
}

uint32_t GetFiberId(){
    return sylar::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip){
    void** array = (void**)malloc(sizeof(void*) * size);
    //该函数用与获取当前线程的调用堆栈,获取的信息将会被存放在buffer中,它是一个指针列表
    size_t s = ::backtrace(array, size);

    //backtrace_symbols将从backtrace函数获取的信息转化为一个字符串数组
    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);

};

std::string BacktraceToString(int size, int skip, const std::string& prefix){
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

uint64_t GetCurrentMS() {
    struct timeval tv;
    //gettimeofday(tv,tz)会把目前的时间用tv 结构体返回，当地时区的信息则放到tz所指的结构中
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

std::string Time2Str(time_t ts, const std::string& format) {
    struct tm tm;
    //localtime用来获取系统时间，精度为秒
    localtime_r(&ts, &tm);
    char buf[64];
    //使用strftime（）函数将时间格式化为我们想要的格式
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

void FSUtil::ListAllFile(std::vector<std::string>& files
                            ,const std::string& path
                            ,const std::string& subfix) {
    //查看路径下的文件是否存在
    if(access(path.c_str(), 0) != 0) {
        SYLAR_LOG_ERROR(g_logger) << "file not exit";
        return;
    }
    //打开文件夹
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "opendir fail";
        return;
    }
    //struct dirent* readdir(DIR* dir);//传文件夹的指针进去，然后返回一个文件项
    struct dirent* dp = nullptr;
    while((dp = readdir(dir)) != nullptr) {
        //如果是文件夹
        if(dp->d_type == DT_DIR) {
            if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }
            ListAllFile(files, path + "/" + dp->d_name, subfix);
        //文件
        } else if(dp->d_type == DT_REG) {
            std::string filename(dp->d_name);
            if(subfix.empty()) {
                files.push_back(path + "/" + filename);
            } else {
                if(filename.size() < subfix.size()) {
                    continue;
                }
                if(filename.substr(filename.length() - subfix.size()) == subfix) {
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    closedir(dir);
}

static int __lstat(const char* file, struct stat* st = nullptr) {
    //int stat(
    //  const char* filename,   //文件或者文件夹的路径
    //  struct stat *buf        //获取的信息保存在内存中
    //)
    struct stat lst;
    //连接文件描述命，获取文件属性,lstat() 函数返回关于文件或符号连接的信息
    int ret = lstat(file, &lst);
    if(st) {
        *st = lst;
    }
    return ret;
}

static int __mkdir(const char* dirname) {
    // access()函数用来判断用户是否具有访问某个文件的权限(或判断某个文件是否存在)
    // F_OK:文件是否存在
    // 对已存在的文件进行读写操作时, 可以先进行访问权限判断
    // R_OK, W_OK, X_OK, 分别是读/写/执行
    if(access(dirname, F_OK) == 0) {
        return 0;
    }
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);//权限依次为读, 写, 执行, 三合一的包含三个权限
}

bool FSUtil::Mkdir(const std::string& dirname) {
    if(__lstat(dirname.c_str()) == 0) {
        return true;
    }
    //char * strdup(const char *s);
    //函数说明：strdup()会先用malloc()配置与参数s 字符串相同的空间大小，
    //然后将参数s 字符串的内容复制到该内存地址，然后把该地址返回。
    //该地址最后可以利用free()来释放
    char* path = strdup(dirname.c_str());
    //char *strchr(const char *str, int c) 在参数 str 所指向的字符串中搜索第一次出现字符 c（一个无符号字符）的位置。
    char* ptr = strchr(path + 1, '/');
    do {
        for(; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if(__mkdir(path) != 0) {
                break;
            }
        }
        if(ptr != nullptr) {
            break;
        } else if(__mkdir(path) != 0) {
            break;
        }
        free(path);
        return true;
    } while(0);
    free(path);
    return false;
}

bool FSUtil::IsRunningPidfile(const std::string& pidfile) {
    if(__lstat(pidfile.c_str()) != 0) {
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if(line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) {
        return false;
    }
    if(kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

}

