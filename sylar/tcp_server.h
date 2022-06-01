#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <memory>
#include <functional>
#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "noncopyable.h"

namespace sylar {

class TcpServer : public std::enable_shared_from_this<TcpServer> 
                    , Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;
    /**
     * @brief 构造函数
     * @param[in] worker socket客户端工作的协程调度器
     * @param[in] accept_worker 服务器socket执行接收socket连接的协程调度器
     */
    TcpServer(sylar::IOManager* worker = sylar::IOManager::GetThis(), 
                sylar::IOManager* accept_worker = sylar::IOManager::GetThis());
    virtual ~TcpServer();
    virtual bool bind(sylar::Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    void setName(const std::string& v) { m_name = v; }

    bool isStop() const { return m_isStop; }
protected:
    //处理新连接的Socket类
    virtual void handleClient(Socket::ptr client);
    //开始接收连接
    virtual void startAccept(Socket::ptr sock);
private:
    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    uint64_t m_recvTimeout;
    std::string m_name;
    bool m_isStop;
};

}

#endif