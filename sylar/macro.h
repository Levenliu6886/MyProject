#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

//__builtin_expect(!!(x), 1)//x很可能为真
//__builtin_expect(!!(x), 0)//x很可能为假
//__buildin_except 函数大致的作用是：向编译器提供分支预测信息，从而帮助编译器进行代码优化
#if defined __GNUC__ || defined __llvm__
#   define SYLAR_LICKLY(x)      __builtin_expect(!!(x), 1)
#   define SYLAR_UNLICKLY(x)    __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LICKLY(x)      (x)
#   define SYLAR_UNLICKLY(x)    (x)
#endif

#define SYLAR_ASSERT(x) \
    if(SYLAR_UNLICKLY(!(x))) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define SYLAR_ASSERT2(x, w) \
    if(SYLAR_UNLICKLY(!(x))) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif 
