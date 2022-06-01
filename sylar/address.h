/**
 *     [UnixAddress]
 *          |
 *      ---------                |-[IPv4Address]
 *      |Address|---[IPAddress]--|
 *      ---------                |-[IPv6Address]
 *          |
 *          |
 *      ---------
 *      | Socket|
 *      ---------
 */

#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/un.h>
#include <vector>
#include <map>

namespace sylar {

class IPAddress;
class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
    //通过host地址返回对应条件的所有Address, host: 域名,服务器名等
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    static Address::ptr LookupAny(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    static std::shared_ptr<IPAddress> LookupAnyIpAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);

    //返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
    static bool GetInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result,
            int family = AF_UNSPEC);
    //获取指定网卡的地址和子网掩码位数
    static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>>& result,
            const std::string& iface, int family = AF_UNSPEC);

    virtual ~Address() {}

    int getFamily() const;

    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrlen() const = 0;

    virtual std::ostream& insert(std::ostream& os) const = 0;
    std::string toString() const;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;//广播地址
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;//网关地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;//子网掩码

    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};

class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;
    //通过域名,IP,服务器名创建IPAddress
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);
    
    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;//广播地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;//网关地址
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;//广播地址
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;//网关地址
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;    
};

class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    void setAddrLen(uint32_t v);
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

}

#endif