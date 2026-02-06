#include <include/lib/x86_index.h>

bool __this_topology_0x1f(u32 *lapic_id, u32 *thread_id,
                          u32 *core_id, u32 *pkg_id)
{
    u32 regs[4] = {0};
    
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);
    if (regs[0] < 0x1f)
        return false;

    cpuid(0x1f, 0, &regs[0], &regs[1], &regs[2], &regs[3]);
    if (regs[1] == 0)
        return false;

    /* edx holds x2apic id */
    u32 full_lapic_id = regs[3];
    u32 smt_shift_width = 0;
    u32 core_shift_width = 0;

    for (u32 i = 0; ; i++) {
        cpuid(0x1f, i, &regs[0], &regs[1], &regs[2], &regs[3]);
        
        u32 level_type = (regs[2] >> 8) & 0xff;
        u32 shift = regs[0] & 0x1f;

        if (level_type == INVAL) 
            break;

        switch (level_type) {

            case SMT:
                smt_shift_width = shift;
                break;

            case CORE:
                core_shift_width = shift;
                break;

            default:
                break;
        }
    }

    if (smt_shift_width != 0)
        *thread_id = full_lapic_id & ((1U << smt_shift_width) - 1);
    else
        *thread_id = 0;

    if (core_shift_width != 0) {
        u32 core_mask = (1U << core_shift_width) - 1;
        *core_id = (full_lapic_id & core_mask) >> smt_shift_width;
        *pkg_id = full_lapic_id >> core_shift_width;
    } else {

        /* handle weird single core per pkg topos */
        *core_id = 0;
        *pkg_id = full_lapic_id >> smt_shift_width;
    }

    *lapic_id = full_lapic_id;
    return true;
}

bool __this_topology_0x0b(u32 *lapic_id, u32 *thread_id, 
                          u32 *core_id, u32 *pkg_id)
{
    /* again, check if its supported and doesnt return nothing */
    u32 regs[4] = {0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);
    if (regs[0] < 0x0b)
        return false;

    cpuid(0x0b, 0, &regs[0], &regs[1], &regs[2], &regs[3]);
    if (regs[1] == 0)
        return false;

    /* edx will hold the lapic id */
    u32 tmp_lapic_id = regs[3];

    /* ecx[15:8] will hold the type 0 - inval, 1 - smt, 2 - core 
       eax[4:0] will hold the shift */
    u32 subleaf0_type = (regs[2] >> 8) & 0xff;
    u32 subleaf0_shift = regs[0] & 0x1f;

    cpuid(0x0b, 1, &regs[0], &regs[1], &regs[2], &regs[3]);
    u32 subleaf1_type = (regs[2] >> 8) & 0xff;
    u32 subleaf1_shift = regs[0] & 0x1f;

    u32 thread_id_shift_width = 0;
    u32 core_id_shift_width = 0;

    if (subleaf0_type == SMT) {

        thread_id_shift_width = subleaf0_shift;

        if (subleaf1_type == CORE && subleaf1_shift != 0)
            core_id_shift_width = subleaf1_shift;

    } else if (subleaf0_type == CORE) {

        core_id_shift_width = subleaf0_shift;
        if (subleaf1_type == SMT && subleaf1_shift != 0)
            thread_id_shift_width = subleaf1_shift; 

    } else {
        return false;
    }

    /* check if were to extract the thread id, which we should if smt is on */
    if (thread_id_shift_width != 0) {
        u32 thread_id_mask = (1U << thread_id_shift_width) - 1;
        *thread_id = tmp_lapic_id & thread_id_mask;
    } else {
        *thread_id = 0;
    }

    /* check if were multi physical core */
    if (core_id_shift_width != 0) {

        /* normal regular shmegular smt shit */
        if (core_id_shift_width > thread_id_shift_width) {

            u32 core_id_mask = (1U << core_id_shift_width) - 1;
            *core_id = (tmp_lapic_id & core_id_mask) >> thread_id_shift_width;
            *pkg_id = tmp_lapic_id >> core_id_shift_width;

        /* weird fucking edge case if its a single core with smt enabled */
        } else if (core_id_shift_width == thread_id_shift_width) {

            *core_id = 0;
            *pkg_id = tmp_lapic_id >> thread_id_shift_width;

        /* shouldnt happen with how shits supposed to be layed out */
        } else {
            return false;
        }

    } else {

        /* single core i guess?? */
        *core_id = tmp_lapic_id;
        *pkg_id = 0;
    }

    *lapic_id = tmp_lapic_id;
    return true;
}

void __this_topology_legacy(u32 *lapic_id, u32 *thread_id, 
                            u32 *core_id, u32 *pkg_id)
{
    u32 regs[4] = {0};
    cpuid(1, 0, &regs[0], &regs[1], &regs[2], &regs[3]);

    /* ebx[31:24] will hold the lapic id for us */
    *lapic_id = (regs[1] >> 24) & 0xff;

    /* ebx[23:16] will tell us the lp count */
    u32 thread_count = (regs[1] >> 16) & 0xff;

    /* check edx[28] to see if smt is supported */
    bool smt_supported = (regs[3] >> 28) & 1;
    
    u32 cores_per_pkg = 1;

    /* we can get the num cores from leaf 4 */
    cpuid(0, 0, &regs[0], &regs[1], &regs[2], &regs[3]);
    if (regs[0] >= 4) {
        cpuid(4, 0, &regs[0], &regs[1], &regs[2], &regs[3]);
        
        /* gotta make sure the cache heirarchy it passes is
           legit so we know its actually supported,
           eax[31:26] will hold the physical core count - 1 */
        if ((regs[0] & 0x1f) != 0)
            cores_per_pkg = ((regs[0] >> 26) & 0x3f) + 1;
    }

    /* gotta check if smt isnt on */
    if (!smt_supported || thread_count < 2) {
        *thread_id = 0;
        *core_id = *lapic_id;
        *pkg_id = 0;
        return;
    }

    /* gotta create bitmasks now so we can derive id's from the lapic id */
    u32 threads_per_core = thread_count / cores_per_pkg;

    u32 thread_id_bits = 0;
    u32 core_id_bits = 0;

    while (threads_per_core > 1) {
        thread_id_bits++;
        threads_per_core >>= 1;
    }

    while (cores_per_pkg > 1) {
        core_id_bits++;
        cores_per_pkg >>= 1;
    }

    u32 thread_id_mask = (1U << thread_id_bits) - 1;
    u32 core_id_mask = (1U << (thread_id_bits + core_id_bits)) - 1;

    /* now we have our masks we can derive the id's, basically 
       it'll be layed out like [pkg_id | core_id | thread_id] */
    *thread_id = *lapic_id & thread_id_mask;
    *core_id = (*lapic_id & core_id_mask) >> thread_id_bits;
    *pkg_id = *lapic_id >> (core_id_bits + thread_id_bits);
}


void this_topology(u32 *lapic_id, u32 *thread_id, u32 *core_id, u32 *pkg_id)
{
    /* grab the core topology with the first leaf that is supported by 
       preference, otherwise it can lead to enumerating wrong info */
    if (__this_topology_0x1f(lapic_id, thread_id, core_id, pkg_id))
        return;

    if (__this_topology_0x0b(lapic_id, thread_id, core_id, pkg_id))
        return;

    __this_topology_legacy(lapic_id, thread_id, core_id, pkg_id);
}

bool may_be_vulnerable_to_its(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    feature_bits_a_t eax = {.val = regs[0]};

    u32 family_id = eax.fields.family_id;
    u32 model_id = eax.fields.model;
    u32 stepping_id = eax.fields.stepping_id;

    if (family_id != 6)
        return false;

    model_id += eax.fields.extended_model_id << 4;

    switch (model_id) {

        case 0x55:

            return stepping_id == 6 || stepping_id == 7 || stepping_id == 0xb;

        case 0x8e:

            return stepping_id == 0xc;

        case 0x9e:

            return stepping_id == 0xd;

        case 0xa5:

            return stepping_id == 2 || stepping_id == 3 || stepping_id == 5;

        case 0xa6:

            return stepping_id <= 1;

        default:
            break;
    }

    return false;
}

bool is_vmx_supported(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    feature_bits_c_t ecx = {.val = regs[2]};
    if (ecx.fields.vmx == 0)
        return false;

    ia32_feature_control_t ctrl = {.val = __rdmsrl(IA32_FEATURE_CONTROL)};

    if (ctrl.fields.locked != 0)
        return ctrl.fields.vmx_outside_smx != 0;

    /* force lock feature control */
    ctrl.fields.vmx_outside_smx = 1;
    ctrl.fields.locked = 1;
    __wrmsrl(IA32_FEATURE_CONTROL, ctrl.val);

    return true;
}