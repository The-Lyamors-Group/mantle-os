#include "mantle.h"

uint64_t mantle_strlen(const char *text)
{
    uint64_t length = 0u;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}

int mantle_streq(const char *left, const char *right)
{
    uint64_t index = 0u;
    while (left[index] != '\0' && left[index] == right[index]) {
        ++index;
    }
    return left[index] == right[index];
}
