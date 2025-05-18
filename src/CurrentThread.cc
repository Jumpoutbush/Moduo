#include "Thread.h"
#include <sys/syscall.h>
#include <unistd.h>
namespace currentThread
{
__thread int t_cachedTid = 0;
void cacheTid()
{
    if(t_cachedTid == 0)    // 未获取过当前线程id
    {
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
}

}