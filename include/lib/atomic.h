#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <std.h>

typedef struct 
{
    long long val;
} atomic64_t __aligned(8);

typedef struct 
{
    int val;
} atomic32_t __aligned(8);

typedef struct 
{
    void *val;
} atomic_ptr_t __aligned(8);

#define INITIALIZE_ATOMIC64(x)  \
{                               \
    .val = (x)                  \
}                         

#define INITIALIZE_ATOMIC32(x)  \
{                               \
    .val = (x)                  \
} 

#define INITIALIZE_ATOMIC_PTR(x) \
{                                \
    .val = (x)                   \
}                                \

inline long long atomic64_xchg(atomic64_t *atomic64, long long val)
{
    return __atomic_exchange_n(&atomic64->val, val, __ATOMIC_ACQ_REL);
}

inline void atomic64_set(atomic64_t *atomic64, long long val)
{
    __atomic_store_n(&atomic64->val, val, __ATOMIC_RELEASE);
}

inline void atomic64_inc(atomic64_t *atomic64)
{
    __atomic_fetch_add(&atomic64->val, 1, __ATOMIC_ACQ_REL);
}

inline void atomic64_dec(atomic64_t *atomic64)
{
    __atomic_fetch_sub(&atomic64->val, 1, __ATOMIC_ACQ_REL);
}

inline void atomic64_add(atomic64_t *atomic64, long long val)
{
    __atomic_fetch_add(&atomic64->val, val, __ATOMIC_ACQ_REL);
}

inline void atomic64_sub(atomic64_t *atomic64, long long val)
{
    __atomic_fetch_sub(&atomic64->val, val, __ATOMIC_ACQ_REL);
}

inline long long atomic64_fetch_and_add(atomic64_t *atomic64, long long val)
{
    return __atomic_fetch_add(&atomic64->val, val, __ATOMIC_ACQ_REL);
}

inline long long atomic64_fetch_and_sub(atomic64_t *atomic64, long long val)
{
    return __atomic_fetch_sub(&atomic64->val, val, __ATOMIC_ACQ_REL);
}

inline bool atomic64_cmpxchg(atomic64_t *atomic64, long long *expected, 
                             long long new_val)
{
    return __atomic_compare_exchange_n(&atomic64->val, expected, new_val,
                                       false, __ATOMIC_ACQ_REL, 
                                       __ATOMIC_ACQUIRE);
}

inline long long atomic64_read(atomic64_t *atomic64)
{
    return __atomic_load_n(&atomic64->val, __ATOMIC_ACQUIRE);
}

inline void atomic32_set(atomic32_t *atomic32, int val)
{
    __atomic_store_n(&atomic32->val, val, __ATOMIC_RELEASE);
}

inline void atomic32_inc(atomic32_t *atomic32)
{
    __atomic_fetch_add(&atomic32->val, 1, __ATOMIC_ACQ_REL);
}

inline void atomic32_dec(atomic32_t *atomic32)
{
    __atomic_fetch_sub(&atomic32->val, 1, __ATOMIC_ACQ_REL);
}

inline void atomic32_add(atomic32_t *atomic32, int val)
{
    __atomic_fetch_add(&atomic32->val, val, __ATOMIC_ACQ_REL);
}

inline void atomic32_sub(atomic32_t *atomic32, int val)
{
    __atomic_fetch_sub(&atomic32->val, val, __ATOMIC_ACQ_REL);
}

inline int atomic32_fetch_and_add(atomic32_t *atomic32, int val)
{
    return __atomic_fetch_add(&atomic32->val, val, __ATOMIC_ACQ_REL);
}

inline bool atomic32_cmpxchg(atomic32_t *atomic32, int *expected, int new_val)
{
    return __atomic_compare_exchange_n(&atomic32->val, expected, new_val,
                                      false, __ATOMIC_ACQ_REL, 
                                      __ATOMIC_ACQUIRE);
}

inline int atomic32_read(atomic32_t *atomic32)
{
    return __atomic_load_n(&atomic32->val, __ATOMIC_ACQUIRE);
}

inline void *atomic_ptr_xchg(atomic_ptr_t *atomic_ptr, void *val)
{
    return __atomic_exchange_n(&atomic_ptr->val, val, __ATOMIC_ACQ_REL);
}

inline void atomic_ptr_set(atomic_ptr_t *atomic_ptr, void *val)
{
    __atomic_store_n(&atomic_ptr->val, val, __ATOMIC_RELEASE);
}

inline void *atomic_ptr_read(atomic_ptr_t *atomic_ptr)
{
    return __atomic_load_n(&atomic_ptr->val, __ATOMIC_ACQUIRE);
}

inline bool atomic_ptr_cmpxchg(atomic_ptr_t *atomic_ptr, void **expected, 
                               void *new_val)
{
    return __atomic_compare_exchange_n(&atomic_ptr->val, expected, new_val,
                                       false, __ATOMIC_ACQ_REL, 
                                       __ATOMIC_ACQUIRE);
}

inline int atomic32_fetch_and_and(atomic32_t *atomic32, int val) 
{
    return __atomic_fetch_and(&atomic32->val, val, __ATOMIC_ACQ_REL);
}

#endif