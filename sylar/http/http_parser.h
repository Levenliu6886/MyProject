#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include "http.h"

#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar {
namespace http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

    size_t execute(char* data, size_t len);//解析协议
    int isFinished();
    int hasError();

    HttpRequest::ptr getData() const { return m_data; }
    void setError(int v) { m_error = v; }

    uint64_t getContentLength();
    const http_parser& getParser() const { return m_parser; }
public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    //1000: invalid method
    //1001: invalid version
    //1002: invalid field
    int m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();
    //有时候，Web服务器生成HTTP Response是无法在Header就确定消息大小的，
    //这时一般来说服务器将不会提供Content-Length的头信息，
    //而采用Chunked编码动态的提供body内容的长度
    size_t execute(char* data, size_t len, bool chunck);//解析协议
    int isFinished();
    int hasError();

    HttpResponse::ptr getData() const { return m_data; }
    void setError(int v) { m_error = v;}

    uint64_t getContentLength();
    const httpclient_parser& getParser() const { return m_parser; }
public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;
};

}
}

#endif