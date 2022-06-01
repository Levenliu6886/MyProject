#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "../socket_stream.h"
#include "http.h"

namespace sylar {
namespace http {

class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;
    //owner:是否托管
    HttpSession(Socket::ptr sock, bool owner = true);
    HttpRequest::ptr recvRequest();//处理请求
    int sendResponse(HttpResponse::ptr rsp);//发送响应

};

}

}

#endif