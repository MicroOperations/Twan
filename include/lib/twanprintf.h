#ifndef _TWANPRINTF_H_
#define _TWANPRINTF_H_

#include <stdarg.h>
#include <types.h>

int twan_vsnprintf(char *restrict buffer, size_t bufsz,
                  char const *restrict format,
                  va_list vlist);

#endif