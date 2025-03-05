#ifndef INFRAX_ASYNC_H
#define INFRAX_ASYNC_H
#include "internal/infrax/InfraxCore.h"
#include "internal/infrax/InfraxMemory.h"

/** DESIGN NOTES

design pattern: factory
main idea: mux (poll + callback)

poll() 是一个多路复用 I/O 机制
可以同时监控多个文件描述符的状态变化
主要监控读、写、异常三种事件
可被 poll() 监控的主要 fd 类型:
a) 网络相关
TCP sockets
UDP sockets
Unix domain sockets
b) 标准 I/O
管道(pipes)
FIFO
终端设备(/dev/tty)
标准输入输出(stdin/stdout/stderr)
c) 其他
字符设备
事件fd (eventfd) linux
定时器fd (timerfd) linux
信号fd (signalfd)
inotify fd (文件系统事件监控)
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <poll.h>

// Forward declarations
typedef struct InfraxAsync InfraxAsync;
typedef struct InfraxPollset InfraxPollset;
typedef struct InfraxPollInfo InfraxPollInfo;
typedef struct InfraxAsyncClassType InfraxAsyncClassType;

// Async states
typedef enum {
    INFRAX_ASYNC_PENDING = 0,
    INFRAX_ASYNC_TMP,     // inner state (see .c)
    INFRAX_ASYNC_FULFILLED,
    INFRAX_ASYNC_REJECTED
} InfraxAsyncState;

// Async callback type
typedef void (*InfraxAsyncCallback)(InfraxAsync* self, void* arg);

// Poll callback type
typedef void (*InfraxPollCallback)(InfraxAsync* self, int fd, short events, void* arg);

// Thread-local pollset
extern __thread struct InfraxPollset* g_pollset;

// Async structure
struct InfraxAsync {
    InfraxAsync* self;
    InfraxAsyncClassType* klass;//InfraxAsyncClass

    InfraxAsyncState state;
    InfraxAsyncCallback callback;
    void* arg;

    InfraxError error;//for reject
    void* result;//for resolve
};

// Class interface
typedef struct InfraxAsyncClassType {
    InfraxAsync* (*new)(InfraxAsyncCallback callback, void* arg);
    void (*free)(InfraxAsync* self);
    bool (*start)(InfraxAsync* self);
    void (*cancel)(InfraxAsync* self);//reject(), TODO resolve()?
    bool (*is_done)(InfraxAsync* self);// fulfilled | rejected

    // pollset operations
    int (*pollset_add_fd)(InfraxAsync* self, int fd, short events, InfraxPollCallback callback, void* arg);
    void (*pollset_remove_fd)(InfraxAsync* self, int fd);
    int (*pollset_poll)(InfraxAsync* self, int timeout_ms);
    
    // Timer operations helper
    InfraxU32 (*setTimeout)(InfraxU32 interval_ms, InfraxPollCallback handler, void* arg);
    InfraxError (*clearTimeout)(InfraxU32 timer_id);
    InfraxU32 (*setInterval)(InfraxU32 interval_ms, InfraxPollCallback handler, void* arg);
    InfraxError (*clearInterval)(InfraxU32 timer_id);
} InfraxAsyncClassType;

// Global class instance
extern InfraxAsyncClassType InfraxAsyncClass;

#endif // INFRAX_ASYNC_H
