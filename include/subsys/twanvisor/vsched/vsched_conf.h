#ifndef _VSCHED_CONF_H_
#define _VSCHED_CONF_H_

#include <include/generated/autoconf.h>
#include <include/compiler.h>
#include <include/types.h>
#include <stdint.h>

#define VEXIT_STACK_SIZE 2048

#if CONFIG_TWANVISOR_ON

#define VSCHED_NUM_CRITICALITIES CONFIG_TWANVISOR_VSCHED_NUM_CRITICALITIES

#else

#define VSCHED_NUM_CRITICALITIES 1

#endif

STATIC_ASSERT(VSCHED_NUM_CRITICALITIES >= 1 &&
              VSCHED_NUM_CRITICALITIES <= UINT8_MAX);

#endif