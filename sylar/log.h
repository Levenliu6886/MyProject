#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <functional>
#include <cstdarg>
#include <map>
#include <set>
#include "util.h"
#include "singleton.h"
#include "thread.h"

#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger,level, \
        __FILE__,__LINE__,0,sylar::GetThreadId(), \
                sylar::GetFiberId(),time(0), sylar::Thread::GetName()))).getSS() 

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar
{ //为了区分和别人的代码重命名的问题

    class Logger;
    class LogEventWarp;
    class LoggerManager;

    //日志级别
    class LogLevel
    {
    public:
        enum Level
        { //日志级别  //enum 枚举变量{枚举值表}
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

        static const char *ToString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string& str);
    };
    
    //日志事件
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line,uint32_t elapse,
                uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string& thread_name);//__FILE__：当前文件名的字符串，__LINE__:当前行号的整数
      

        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapes() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        const std::string& getThreadName() const { return m_threadName; }
        std::string getContent() const { return m_ss.str(); }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }

        std::stringstream& getSS() { return m_ss;}
        void format(const char* fmt, ...);          //格式化写入日志内容
        void format(const char* fmt,va_list al);

    private:
        const char *m_file = nullptr; //文件名
        int32_t m_line = 0;           //行号
        uint32_t m_elapse = 0;        //程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;      //线程号
        uint32_t m_fiberId = 0;       //协程号
        uint32_t m_time = 0;          //时间戳
        std::string m_threadName;
        std::stringstream m_ss;        //消息

        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };

    class LogEventWrap {//放LoggerEvent
    public:
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();
        LogEvent::ptr getEvent() const { return m_event;}
        std::stringstream& getSS();
    private:
        LogEvent::ptr m_event;
    };


    //日志格式器
    class LogFormatter
    { //每个日志输出目的地的格式不一样//格式化程序
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        //pattern赋值的时候我们会根据pattern的格式来解析出它里面的item们的信息
        LogFormatter(const std::string &pattern);

        //%t    %thread_id %m%n（固定格式） //怎么样实现把事件解析成固定化格式来输出我们的日志
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event); //把它变成一个string提供给appender去输出

    public:
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            //FormatItem(const std::string &fmt = "");
            virtual ~FormatItem() {}
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();

        bool isError() const { return m_error;}
        const std::string getPattern() const { return m_pattern; }
    private:
        //这里会定义很多具体的子类 会负责输出的其中一部分
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    //日志输出地
    class LogAppender
    {
    friend class Logger;
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        typedef Spinlock MutexType;
        virtual ~LogAppender(){};

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        virtual std::string toYamlString() = 0;

        void setFormatter(LogFormatter::ptr val);
        LogFormatter::ptr getFormatter();

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;       //主要针对哪些日志定义的级别
        bool m_hasFormatter = false;
        MutexType m_mutex;
        LogFormatter::ptr m_formatter; //输出的格式会有不同
    };

    //日志器
    class Logger : public std::enable_shared_from_this<Logger>//只有继承这个它才能在自己的成员函数获得自己的指针,才能把自己作为智能指针传上去
    {
    friend class LoggerManager;
    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;

        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void unknow(LogEvent::ptr event);
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);
        void error(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender); //添加日志输出地
        void delAppender(LogAppender::ptr appender); //删除日志输出地
        void clearAppenders();
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string &getName() const { return m_name; }

        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string& val);

        LogFormatter::ptr getFormatter();

        std::string toYamlString();
    private:
        std::string m_name;                      //日志名称
        LogLevel::Level m_level;                 //日志器的日志级别//之后满足了这个日志级别的日志才会被输出到里面
        MutexType m_mutex;
        std::list<LogAppender::ptr> m_appenders; //Appender是个列表，是输出到目的地的集合
        LogFormatter::ptr m_formatter;
        Logger::ptr m_root;
    };

    //输出到控制台的Appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;
    };

    //定义到输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename); //因为是输出到文件，因此还需要文件名
        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
        std::string toYamlString() override;
        //重新打开文件，文件打开成功返回true
        bool reopen(); //文件涉及到文件的打开或者重新打开
    private:
        std::string m_filename;
        std::ofstream m_filestream; //要包含头文件sstream fstream
        uint64_t m_lastTime = 0;
    };

    class LoggerManager{
    public:
        typedef Spinlock MutexType;
        LoggerManager();
        Logger::ptr getLogger(const std::string& name);

        void init();
        Logger::ptr getRoot() const { return m_root; }
        std::string toYamlString();
    private:
        MutexType m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;

    };

    typedef sylar::Singleton<LoggerManager> LoggerMgr;


}

#endif