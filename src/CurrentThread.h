#pragma once

#include <sys/syscall.h>
#include <unistd.h>

namespace currentThread
{
extern __thread int t_cachedTid;

void cacheTid();

inline int tid()
{
    if(__builtin_expect(t_cachedTid == 0, 0))   // 未获取过当前线程id
    {
        cacheTid();
    }
    return t_cachedTid;
}

}