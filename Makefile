ARMGNU ?= arm-none-eabi-

CC = $(ARMGNU)gcc
AS = $(ARMGNU)as
LD = $(ARMGNU)ld
OBJCOPY = $(ARMGNU)objcopy
GDB = $(ARMGNU)gdb

SRCDIR = src
INCDIR = include
BUILDDIR = build

TARGET = kernel.img
ELFTARGET = $(BUILDDIR)/kernel.elf

SOURCES_C = $(wildcard $(SRCDIR)/**/*.c)
SOURCES_S = startup.s
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES_C))
OBJECTS += $(patsubst %.s, $(BUILDDIR)/%.o, $(SOURCES_S))

CFLAGS = -nostdlib -ffreestanding -I$(INCDIR) -g -O0 -Wall -Wextra -march=armv7-a -mtune=cortex-a7

ASFLAGS = -g -march=armv7-a

all: $(TARGET)
	@echo "  BUILDING  $(TARGET)"
	@echo "  DONE"

$(TARGET): $(ELFTARGET)
	@echo "  OBJCOPY  $(ELFTARGET) -> $(TARGET)"
	@$(OBJCOPY) $(ELFTARGET) -O binary $(TARGET)

$(ELFTARGET): $(OBJECTS) linker.ld
	@echo "  LD       $@ -> $(ELFTARGET)"
	@$(LD) -T linker.ld -o $(ELFTARGET) $(OBJECTS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC       $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.s
	@mkdir -p $(BUILDDIR)
	@echo "  AS       $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILDDIR) $(TARGET)

run-qemu: all
	@echo "  RUNNING  QEMU"
	@qemu-system-arm -M raspi2b -kernel $(TARGET) -chardev stdio,id=char0 -device aux-uart,chardev=char0

debug-qemu: all
	@qemu-system-arm -M raspi2b -kernel $(ELFTARGET) -chardev stdio,id=char0 -device aux-uart,chardev=char0 -S -s

.PHONY: all clean run-qemu debug-qemu
