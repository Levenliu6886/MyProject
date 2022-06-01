#ifndef __SYLAR_URI_H__
#define __SYLAR_URI_H__

#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"

namespace sylar {

/*
     foo://user@host:port/over/there?name=ferret#nose
     \_/\______________/\_________/ \_________/ \__/
      |           |            |         |        |
     scheme     authority       path     query   fragment
*/
class Uri {
public:
    typedef std::shared_ptr<Uri> ptr;

    static Uri::ptr Create(const std::string& uristr);
    Uri();
    const std::string& getScheme() const { return m_scheme; }
    const std::string& getUserinfo() const { return m_userinfo; }
    const std::string& getHost() const { return m_host; }
    const std::string& getQuery() const { return m_query; }
    const std::string& getFragment() const { return m_fragment; }
    //int32_t getPort() const { return m_port; }

    void setScheme(const std::string& v) { m_scheme = v; }
    void setUserinfo(const std::string& v) { m_userinfo = v; }
    void setHost(const std::string& v) { m_host = v; }
    void setPath(const std::string& v) { m_path = v; }
    void setQuery(const std::string& v) { m_query = v; }
    void setFragment(const std::string& v) { m_fragment = v; }
    void setPort(const int32_t v) { m_port = v; }

    const std::string& getPath() const;
    int32_t getPort() const;

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
    Address::ptr createAddress() const;
private:
     bool isDefaultPort() const;
private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    int32_t m_port;
};

}

#endif