#ifndef _VSCHED_CONF_H_
#define _VSCHED_CONF_H_

#include <subsys/twanvisor/vconf.h>
#include <compiler.h>
#include <types.h>
#include <stdint.h>

#define VEXIT_STACK_SIZE 2048

#if TWANVISOR_ON

#define VSCHED_NUM_CRITICALITIES CONFIG_TWANVISOR_VSCHED_NUM_CRITICALITIES

#else

#define VSCHED_NUM_CRITICALITIES 1

#endif

STATIC_ASSERT(VSCHED_NUM_CRITICALITIES >= 1 &&
              VSCHED_NUM_CRITICALITIES <= UINT8_MAX);

#endif