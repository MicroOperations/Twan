#ifndef _VSHIELD_H_
#define _VSHIELD_H_

#include <types.h>

int venter_shield_local(u64 *flags, u8 vector, u64 rip);
int venter_shield_local_length(u64 *flags, u8 vector, u8 length);
bool vexit_shield_local(u64 flags);

#endif