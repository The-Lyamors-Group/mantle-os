#include "libc/mantle.h"

int main(void)
{
    static const char started[] = "MANTLE_INIT_USER_OK\n";
    static const char failed[] = "MANTLE_INIT_SHELL_ERROR\n";
    mantle_write(1u, started, sizeof(started) - 1u);
    if (mantle_exec("/bin/mantle-shell") < 0) {
        mantle_write(1u, failed, sizeof(failed) - 1u);
    }
    return 1;
}
