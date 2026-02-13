#ifndef _COMPILER_H_
#define _COMPILER_H_

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L && \
    !defined(__cplusplus)

    #define VERSION_C23 1

#else

    #define VERSION_C23 0
    
#endif

#define __aligned(x) __attribute__((aligned(x)))
#define __packed __attribute__((packed))
#define __noreturn __attribute__((noreturn))
#define __always_inline __attribute__((always_inline))
#define __used __attribute__((used))
#define __unused __attribute__((unused))
#define __fallthrough __attribute__((fallthrough))
#define __unroll_loops __attribute__((optimize("unroll-loops")))

#define STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)

#define SIZE_ASSERT(obj, size) \
    STATIC_ASSERT(sizeof(obj) == (size), "size mismatch: " #obj)

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define ENTRY_COUNT(table) ARRAY_LEN((table))

#define CONCAT(x, y) x##y
#define EXPAND_CONCAT(x, y) CONCAT(x, y)

#define num_needed(val, size) (((val) + (size) - 1) / (size))
#define is_pow2(x) (((x) & ((x) - 1)) == 0)

#define TEST_BYTE 0x69

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))

#define log2_ceil32(x) \
    ((x) <= 1 ? 0 : 32 - __builtin_clz((x) - 1))

#define log2_floor32(x) ((x) ? 31 - __builtin_clz((x)) : 0)
#define log2_floor64(x) ((x) ? 63 - __builtin_clz((x)) : 0)

#define log2_reverse(x) ((x) < 2 ? 0 : 64 - __builtin_clzll((x) - 1))

#define p2align_up(x, y) (((x) + (y) - 1) & ~((y) - 1))
#define p2align_down(x, y) ((x) & ~((y) - 1))

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define return_address() __builtin_return_address(0)

#define clz(x) (__builtin_clz((x)))

#define FEMTOSECOND 1000000000000000ULL

#ifndef barrier

#define barrier() __asm__ __volatile__("": : :"memory")

#endif

#endif