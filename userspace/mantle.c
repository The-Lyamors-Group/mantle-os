#include "libc/mantle.h"

int main(void)
{
    static const char version[] = "MantleOS mantle 0.1\n";
    mantle_write(1u, version, sizeof(version) - 1u);
    return 0;
}
