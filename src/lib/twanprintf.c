#include <lib/twanprintf.h>
#include <nanoprintf.h>

int twan_vsnprintf(char *restrict buffer, size_t bufsz,
                  char const *restrict format,
                  va_list vlist)
{
    return npf_vsnprintf(buffer, bufsz, format, vlist);
}