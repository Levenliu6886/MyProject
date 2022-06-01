#include "../sylar/sylar.h"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert() {
    //assert的作用是现计算表达式 expression ，如果其值为假（即为0），
    //那么它先向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行。
    //assert(0);

    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    SYLAR_ASSERT2(0 == 1, "abcdef xx");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}