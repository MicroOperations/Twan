CC = gcc
LD = ld
PY = python3
KCONFIG_CONFIG = .config
KCONFIG_CONFIG_OLD = .config.old

export KCONFIG_CONFIG

SCRIPTS_DIR = scripts
GEN_DIR = include/generated

GENCONFIG_SRC = $(SCRIPTS_DIR)/genconfig.py
AUTOCONF_H = $(GEN_DIR)/autoconf.h

INCLUDE_DIR = include/
SRC_DIR = src
BOOT_CONF_DIR = boot
GRUB_DIR = $(BOOT_CONF_DIR)/grub
UACPI_DIR = uACPI
BUILD_DIR = build
ISO_DIR = iso_root

KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO_NAME = TwanRTOS.iso

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
C_SOURCES = $(call rwildcard,$(SRC_DIR)/,*.c)
ASM_SOURCES = $(call rwildcard,$(SRC_DIR)/,*.S)
UACPI_SOURCES = $(call rwildcard,$(UACPI_DIR)/src/,*.c)

OBJS = $(patsubst %.S, $(BUILD_DIR)/%.o, $(ASM_SOURCES)) \
		$(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) \
	    $(patsubst %.c, $(BUILD_DIR)/%.o, $(UACPI_SOURCES))

UACPI_CFLAGS = -I$(UACPI_DIR)/include/
CFLAGS = -O2 -mstackrealign -ffreestanding -Wall -Wextra -fno-stack-protector -m64 \
		-I$(INCLUDE_DIR) $(UACPI_CFLAGS)
LDFLAGS = -T linker.ld -nostdlib

# default

all: $(AUTOCONF_H) $(ISO_NAME)

# config 

$(KCONFIG_CONFIG):
	$(PY) -m alldefconfig

$(AUTOCONF_H): $(KCONFIG_CONFIG)
	@mkdir -p $(GEN_DIR)
	$(PY) $(GENCONFIG_SRC) > $(AUTOCONF_H)

defconfig: $(KCONFIG_CONFIG)

menuconfig:
	$(PY) -m menuconfig
	@mkdir -p $(GEN_DIR)
	$(PY) $(GENCONFIG_SRC) > $(AUTOCONF_H)

genconfig: $(AUTOCONF_H)

# build

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(ISO_NAME): $(KERNEL_ELF) $(GRUB_DIR)/grub.cfg
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/twan
	cp $(GRUB_DIR)/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)

# utilities

qemu: $(AUTOCONF_H) $(ISO_NAME)
	qemu-system-x86_64 -cdrom $(ISO_NAME) -serial stdio -m 512M -smp 4
	
kvm: $(AUTOCONF_H) $(ISO_NAME)
	sudo qemu-system-x86_64 -enable-kvm -cpu host \
		-cdrom $(ISO_NAME) -serial stdio -m 512M -smp 4

# cleanup

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO_NAME)

distclean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO_NAME) $(KCONFIG_CONFIG) $(KCONFIG_CONFIG_OLD) $(GEN_DIR)

.PHONY: all defconfig menuconfig genconfig clean distclean qemu kvm