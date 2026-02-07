ENTRY(_start)

SECTIONS {
    . = 1M;

    __kernel_start = .;

    .boot : {
        *(.multiboot_header)
    }

    .driver_registry : ALIGN(64) {
        __driver_registry_start = .;
        KEEP(*(.driver_registry))
        __driver_registry_end = .;
    }

    .early_initcall_registry : ALIGN(64) {
        __early_initcall_registry_start = .;
        KEEP(*(SORT(.early_initcall_registry.*)))
        __early_initcall_registry_end = .;
    }

    .late_initcall_registry : ALIGN(64) {
        __late_initcall_registry_start = .;
        KEEP(*(SORT(.late_initcall_registry.*)))
        __late_initcall_registry_end = .;
    }

    .vma_partition_registry : ALIGN(64) {
        __vma_partition_registry_start = .;
        KEEP(*(.vma_partition_registry))
        __vma_partition_registry_end = .;
    }

    .text : ALIGN(4K) {
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    }

    __kernel_end = .;
}

ASSERT(__kernel_end <= ((1 << 30)), "kernel image too large (must fit in low 1MB - 1GB)");
