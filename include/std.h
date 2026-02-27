#ifndef _STD_H_
#define _STD_H_

#include <generated/autoconf.h>
#include <stddef.h>
#include <types.h>
#include <lib/x86_index.h>

#define PAGE_SIZE PAGE_SIZE_2MB
#define PAGE_SHIFT PAGE_SHIFT_2MB

#if CONFIG_MITIGATION_INTRAMODE_BTI

/* needed for mitigations not provided by compiler flags */
#define INDIRECT_BRANCH_SAFE(code) ({ \
    ibhf_stub();                      \
    (code);                           \
})

#else 

#define INDIRECT_BRANCH_SAFE(code) ({ \
    (code);                           \
})

#endif

#define spin_until(cond)    \
    do {                   \
        while (!(cond))    \
            cpu_relax();   \
    } while (0)            \

#define UNREACHABLE() do {      \
    __hlt();                    \
} while (1)                     \

typedef union 
{
    u8 val;
    struct
    {
        u8 intl : 4;
        u8 reserved0 : 4;
    } fields;
} intl_t;

struct memory_range
{
    u64 start;
    size_t size;
};

#define INTL_MIN 0
#define INTL_MAX 15
#define NUM_INTLS 16

#define vector_to_intl(vector) ((vector) / NUM_INTLS)

#define ms_to_ticks(ms, freq_hz) (((ms) * (freq_hz)) / 1000ULL)
#define us_to_ticks(us, freq_hz) (((us) * (freq_hz)) / 1000000ULL)

#define READ_ONCE(x)					                        \
({      					                                    \
	union {typeof((x)) __val; char __c[1];} __u = {.__c = {0}}; \
                                                                \
	__read_once_size(&(x), __u.__c, sizeof((x)));	            \
                                                                \
	__u.__val;					                                \
})

#define WRITE_ONCE(x, val)				                                \
({							                                            \
	union {typeof((x)) __val; char __c[1];} __u = {.__val = (val)};     \
                                                                        \
	__write_once_size(&(x), __u.__c, sizeof((x)));	                    \
                                                                        \
	__u.__val;					                                        \
})

inline bool is_canonical(u64 va)
{
    u64 bit = ((va >> 47) & 1) != 0;

    return bit ? va >> 48 == 0xffff : va >> 48 == 0;
}

inline u64 log2(size_t size)
{
    if (size == 0)
        return 0;

    u64 order = 0;
    u64 temp = size >> 1;
    
    while (temp != 0) {
        order++;
        temp >>= 1;
    }
    
    if (!is_pow2(size))
        order++;
    
    return order;
}

inline int fls32(u32 val)
{
    if (val == 0)
        return -1;

    int bit = 31;

    if (!(val & 0xffff0000)) {
        val <<= 16;
        bit -= 16;
    }

    if (!(val & 0xff000000)) {
        val <<= 8;
        bit -= 8;
    }

    if (!(val & 0xf0000000)) {
        val <<= 4;
        bit -= 4;
    }

    if (!(val & 0xc0000000)) {
        val <<= 2;
        bit -= 2;
    }

    if (!(val & 0x80000000))
        bit -= 1;

    return bit;
}

inline int ffs32(u32 val) 
{
    if (val == 0)
        return -1;  
        
    int bit = 0;
    if (!(val & 0x0000ffff)) {
        val >>= 16;
        bit += 16;
    }
    if (!(val & 0x000000ff)) {
        val >>= 8;
        bit += 8;
    }
    if (!(val & 0x0000000f)) {
        val >>= 4;
        bit += 4;
    }
    if (!(val & 0x00000003)) {
        val >>= 2;
        bit += 2;
    }
    if (!(val & 0x00000001))
        bit += 1;
        
    return bit;
}

inline int fls64(u64 val)
{
    if (val == 0)
        return -1;

    int bit = 63;

    if (!(val & 0xffffffff00000000)) {
        val <<= 32;
        bit -= 32;
    }

    if (!(val & 0xffff000000000000)) {
        val <<= 16;
        bit -= 16;
    }

    if (!(val & 0xff00000000000000)) {
        val <<= 8;
        bit -= 8;
    }

    if (!(val & 0xf000000000000000)) {
        val <<= 4;
        bit -= 4;
    }

    if (!(val & 0xc000000000000000)) {
        val <<= 2;
        bit -= 2;
    }

    if (!(val & 0x8000000000000000))
        bit -= 1;

    return bit;
}

inline int ffs64(u64 val) 
{
    if (val == 0)
        return -1;  
        
    int bit = 0;

    if (!(val & 0x00000000ffffffff)) {
        val >>= 32;
        bit += 32;
    }

    if (!(val & 0x000000000000ffff)) {
        val >>= 16;
        bit += 16;
    }

    if (!(val & 0x00000000000000ff)) {
        val >>= 8;
        bit += 8;
    }

    if (!(val & 0x000000000000000f)) {
        val >>= 4;
        bit += 4;
    }

    if (!(val & 0x0000000000000003)) {
        val >>= 2;
        bit += 2;
    }

    if (!(val & 0x0000000000000001))
        bit += 1;
        
    return bit;
}

inline int strncmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;

	do {

		if (*s1 != *s2++)
			return *(unsigned char *)s1 - *(unsigned char *)--s2;

		if (*s1++ == 0)
			break;

	} while (--n != 0);
    
	return 0;
}

inline void *memset(void *str, int c, size_t n)
{
    char *_str = str;

    for (u64 i = 0; i < n; i++)
        _str[i] = c;

    return str;
}

inline void *memcpy(void *dest, const void *src, size_t n)
{
    const char *_src = src;
    char *_dest = dest;

    for (u64 i = 0; i < n; i++)
        _dest[i] = _src[i]; 

    return dest;
}

inline u8 inb(u16 port)
{
    return __inb(port);
}

inline u16 inw(u16 port)
{
    return __inw(port);
}

inline u32 inl(u16 port)
{
    return __inl(port);
}

inline void outb(u16 port, u8 val)
{
    __outb(port, val);
}

inline void outw(u16 port, u16 val)
{
    __outw(port, val);
}

inline void outl(u16 port, u32 val)
{
    __outl(port, val);
}

inline void io_wait(void)
{
    __io_wait();
}

inline void disable_interrupts(void)
{
    __cli();
}

inline void enable_interrupts(void)
{
    __sti();
}

inline void halt_loop(void)
{
    __hlt_loop();
}

inline bool is_interrupts_enabled(void)
{
    return __read_rflags().fields._if != 0;
}

inline u64 read_flags(void)
{
    return __read_rflags().val;
}

inline void write_flags(u64 val)
{
    rflags_t rflags = {.val = val};
    __write_rflags(rflags);
}

inline u64 read_flags_and_disable_interrupts(void)
{
    u64 flags = read_flags();
    disable_interrupts();

    return flags;
}

inline void flags_set_interrupts_enabled(u64 *flags)
{
    rflags_t rflags = {.val = *flags};
    rflags.fields._if = 1;
    *flags = rflags.val;
}

inline void flags_set_interrupts_disabled(u64 *flags)
{
    rflags_t rflags = {.val = *flags};
    rflags.fields._if = 0;
    *flags = rflags.val;
}

inline bool flags_is_interrupts_enabled(u64 flags)
{
    rflags_t rflags = {.val = flags};
    return rflags.fields._if != 0;
}

inline void cpu_relax(void)
{
    __pause();
}

inline void flush_this_tlb(void)
{
    __flush_tlb();
}

inline void flush_this_tlb_page(u64 addr)
{
    __invlpg((void *)addr);
}

inline void flush_this_tlb_range(u64 first, u64 last)
{
    u64 aligned_start = p2align_down(first, PAGE_SIZE);
    u64 aligned_end = p2align_down(last, PAGE_SIZE);

    for (u64 i = aligned_start; i != aligned_end; i += PAGE_SIZE)
        flush_this_tlb_page(i);

    flush_this_tlb_page(aligned_end);
}

inline intl_t read_intl(void)
{
    intl_t intl = {0};
    cr8_t cr8 = __read_cr8();

    intl.fields.intl = cr8.fields.tpr;
    return intl;
}

inline void write_intl(intl_t intl)
{
    cr8_t cr8 = __read_cr8();
    cr8.fields.tpr = intl.fields.intl;
    __write_cr8(cr8);
}

inline bool inc_intl(void)
{
    intl_t intl = read_intl();
    if (intl.fields.intl == INTL_MAX)
        return false;

    intl.fields.intl++;
    write_intl(intl);

    return true;
}

inline bool dec_intl(void)
{
    intl_t intl = read_intl();
    if (intl.fields.intl == INTL_MIN)
        return false;

    intl.fields.intl--;
    write_intl(intl);
    
    return true;
}

inline u8 bcd_to_decimal(u8 bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0xf);
}

inline u8 decimal_to_bcd(u8 dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

inline void __read_once_size(const volatile void *p, void *res, int size)
{
	switch (size) {
        
	    case 1: *(u8  *) res = *(volatile u8  *)p; break;
	    case 2: *(u16 *) res = *(volatile u16 *)p; break;
	    case 4: *(u32 *) res = *(volatile u32 *)p; break;
	    case 8: *(u64 *) res = *(volatile u64 *)p; break;
	    default:
		    barrier();
		    memcpy((void *)res, (const void *)p, size);
		    barrier();
            break;
	    }
}

inline void __write_once_size(volatile void *p, void *res, int size)
{
	switch (size) {

	    case 1: *(volatile  u8 *) p = *(u8  *)res; break;
	    case 2: *(volatile u16 *) p = *(u16 *)res; break;
	    case 4: *(volatile u32 *) p = *(u32 *)res; break;
	    case 8: *(volatile u64 *) p = *(u64 *)res; break;
	    default:
		    barrier();
		    memcpy((void *)p, (const void *)res, size);
		    barrier();
            break;
	}
}

#endif