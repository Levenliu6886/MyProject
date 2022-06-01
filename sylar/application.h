/**
 * 1、防止重复启动多次(pid)
 * 2、初始化日志文件路径(/path/to/log)
 * 3、工作目录的路径(/path/to/work)
 * 
 * 4、解析httpserver配置，通过配置，启动httpserver
 */
#ifndef __SYLAR_APPLICATION_H__
#define __SYLAR_APPLICATION_H__

#include "sylar/http/http_server.h"

namespace sylar {

class Application {
public:
    Application();

    static Application* GetInstance() { return s_instance; }
    bool init(int argc, char** argv);
    bool run();

    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs);
    void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);


private:
    int main(int argc, char** argv);
    int run_fiber();
private:
    int m_argc = 0;
    char** m_argv = nullptr;

    std::vector<sylar::http::HttpServer::ptr> m_httpservers;
    //std::map<std::string, std::vector<TcpServer::ptr> > m_servers;
    IOManager::ptr m_mainIOManager;
    static Application* s_instance;

};

}

#endif
