#ifndef __SYLAR_HTTP_HTTP_SERVER_H__
#define __SYLAR_HTTP_HTTP_SERVER_H__

#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace sylar {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false
                ,sylar::IOManager* worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());
    
    ServletDispatch::ptr getServletDispath() const { return m_dispatch; }
    void setServletDispath(ServletDispatch::ptr v) { m_dispatch = v; }
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepalive;//是否支持常连接
    ServletDispatch::ptr m_dispatch;
};

}
}

#endif