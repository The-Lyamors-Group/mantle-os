#include <mantle/types.h>
#include "../kernel/arch/x86_64/kernel.h"

int main(void)
{
    struct mantle_iret_frame frame = {
        0x1bu, 0x600000u, 0x2u, 0x23u, 0x400000u
    };
    if ((frame.ss & 3u) != 3u || (frame.cs & 3u) != 3u ||
        (frame.rflags & 2u) == 0u || (frame.rsp & 0xfu) != 0u) {
        return 1;
    }
    return 0;
}
