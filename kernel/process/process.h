#ifndef MANTLE_KERNEL_PROCESS_H
#define MANTLE_KERNEL_PROCESS_H

#include <mantle/types.h>

enum mantle_process_state {
    MANTLE_PROCESS_READY = 0,
    MANTLE_PROCESS_RUNNING = 1,
    MANTLE_PROCESS_EXITED = 2
};

struct mantle_process {
    uint32_t pid;
    uint32_t ppid;
    uint32_t uid;
    uint32_t gid;
    enum mantle_process_state state;
    int32_t exit_code;
    char cwd[64];
};

struct mantle_process *mantle_current_process(void);
void mantle_process_init(void);

#endif
