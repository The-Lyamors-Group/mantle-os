#include "process.h"

static struct mantle_process current;

void mantle_process_init(void)
{
    current.pid = 1u;
    current.ppid = 0u;
    current.uid = 0u;
    current.gid = 0u;
    current.state = MANTLE_PROCESS_READY;
    current.exit_code = 0;
    current.cwd[0] = '/';
    current.cwd[1] = '\0';
}

struct mantle_process *mantle_current_process(void)
{
    return &current;
}
