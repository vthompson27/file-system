# Define o prefixo da toolchain para compilação cruzada ARM 32-bit
ARMGNU ?= arm-none-eabi-

# Define os compiladores e ferramentas
CC = $(ARMGNU)gcc
AS = $(ARMGNU)as
LD = $(ARMGNU)ld
OBJCOPY = $(ARMGNU)objcopy
GDB = $(ARMGNU)gdb

# Diretórios
SRCDIR = src
INCDIR = include
BUILDDIR = build

# Nome do kernel final para RPi2/3 em modo 32-bit
TARGET = kernel7.img
ELFTARGET = $(BUILDDIR)/kernel7.elf

# Arquivos fonte e objetos
SOURCES_C = $(wildcard $(SRCDIR)/*.c)
SOURCES_S = startup.s
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES_C))
OBJECTS += $(patsubst %.s, $(BUILDDIR)/%.o, $(SOURCES_S))

# Flags de compilação para C
CFLAGS = -nostdlib -ffreestanding -I$(INCDIR) -g -O0 -Wall -Wextra -march=armv7-a -mtune=cortex-a7

# Flags para o Assembler
ASFLAGS = -g -march=armv7-a

# Regra principal: criar o kernel
all: $(TARGET)

# Cria o .img a partir do arquivo ELF
$(TARGET): $(ELFTARGET)
	@echo "  OBJCOPY  $(ELFTARGET) -> $(TARGET)"
	@$(OBJCOPY) $(ELFTARGET) -O binary $(TARGET)

# Linka todos os arquivos objeto para criar o ELF
$(ELFTARGET): $(OBJECTS) linker.ld
	@echo "  LD       $@ -> $(ELFTARGET)"
	@$(LD) -T linker.ld -o $(ELFTARGET) $(OBJECTS)

# Compila arquivos C
$(BUILDDIR)/%.o: $(SRCDIR)%.c
	@mkdir -p $(BUILDDIR)
	@echo "  CC       $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

# Monta arquivos Assembly
$(BUILDDIR)/%.o: %.s
	@mkdir -p $(BUILDDIR)
	@echo "  AS       $< -> $@"
	@$(AS) $(ASFLAGS) $< -o $@

# Regra para limpar os arquivos gerados
clean:
	@echo "  CLEAN"
	@rm -rf $(BUILDDIR) $(TARGET)

# Regra para rodar com QEMU (conectando à UART principal PL011)
run-qemu: all
	@echo "  RUNNING  QEMU"
	@qemu-system-arm -M raspi2b -kernel $(TARGET) -serial stdio

# Regra para depurar com GDB (conectando à UART principal PL011)
debug-qemu: all
	@qemu-system-arm -M raspi2b -kernel $(ELFTARGET) -serial stdio -S -s

.PHONY: all clean run-qemu debug-qemu