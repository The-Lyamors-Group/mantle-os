#include "libc/mantle.h"

int main(void)
{
    static const char message[] = "MantleOS userspace\n";
    mantle_write(1u, message, sizeof(message) - 1u);
    return 0;
}
