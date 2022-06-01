#include "../sylar/sylar.h"
#include "../sylar/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int sock = 0;

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test_fiber";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "112.80.248.76", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)& addr, sizeof(addr))) {
    
    } else if(errno == EINPROGRESS) {//当我们以非阻塞的方式来进行连接的时候，返回的结果如果是 -1,这并不代表这次连接发生了错误，如果它的返回结果是 EINPROGRESS，那么就代表连接还在进行中
        SYLAR_LOG_INFO(g_logger) << "add event errno= " << errno << " " << strerror(errno);
        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ, [](){
            SYLAR_LOG_INFO(g_logger) << "read callback";
        });

        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE, [](){
            SYLAR_LOG_INFO(g_logger) << "write callback";
            sylar::IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::READ);
            close(sock);
        });
    } else {
        SYLAR_LOG_INFO(g_logger) << " else " << errno << " " << strerror(errno);
    }

    

}

void test1() {
    //std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT= " << EPOLLOUT << std::endl;
    sylar::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

// sylar::Timer::ptr s_timer;
// void test_timer() {
//     sylar::IOManager iom(2);
//     s_timer = iom.addTimer(1000, [](){
//         static int i = 0;
//         SYLAR_LOG_INFO(g_logger) << "hello timer";
//         if(++i == 3) {
//             //s_timer->reset(2000, true);
//             s_timer->cancel();
//         }
//     }, true);//1s2个
// } 

int main(int argc, char** argv) {
    //test_timer();
    test1();
    return 0;
}