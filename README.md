# 🧠 Raspberry Pi 3 File System Development

This project is a bare-metal learning environment for developing a simple file system from scratch on the Raspberry Pi 3.

## 🔧 Features

- Runs without an operating system (bare-metal)
- Direct interaction with hardware
- UART serial output for debugging
- Basic memory and I/O routines
- Simple custom file system planned (inode based)

## 🧱 Project Structure

```
.
├── Makefile           # Build system using arm-none-eabi toolchain
├── src/               # Source files
│   ├── kernel.c       # Entry point
│   ├── common.c       # Utility functions
│   ├── shell.c        # Shell operation logic
│   ├── uart.c         # UART operations
│   ├── simplefs.c     # File system logic
│   └── ...
├── include/           # Header files
│   └── common.h/
│   └── shell.h/
│   └── simplefs.h
│   └── uart.h/
├── build/             # Build output directory
├── kernel.img         # Final image to boot on the Pi
├── config.txt         # Boot configuration for Pi firmware
├── bootcode.bin       # GPU bootloader
├── start.elf          # GPU firmware
├── linker.ld          # Linker script
└── README.md
```

## 🚀 Getting Started

### 1. Requirements

- Raspberry Pi 3
- Cross compiler: `arm-none-eabi-gcc`
- USB-to-TTL Serial cable
- Files from a Raspbian image: `bootcode.bin`, `start.elf`

### 2. Build

```bash
make
```

This will generate `kernel.img`.

### 3. SD Card Setup

1. Format SD card as **FAT32** with **MBR partition table**
2. Copy these files to the root of the card:
   - `kernel.img`
   - `config.txt`
   - `bootcode.bin`
   - `start.elf`

Example `config.txt`:

```ini
kernel=kernel.img
arm_64bit=0
disable_commandline_tags=1
kernel_address=0x8000
core_freq=250
```

### 4. Run

1. Insert the SD card into the Pi
2. Connect serial cable to GPIO14/15 (TX/RX)
3. Open a terminal:

```bash
screen /dev/tty.usbserial-XXXXX 115200
```

4. Power on the Pi and watch for UART output

---

## 📁 Roadmap

- [x] UART and GPIO working
- [x] Basic memory routines
- [x] Disk block abstraction
- [x] File system layout (in-memory prototype)
- [x] Directory structure and file metadata
- [x] Read/write file operations
- [ ] Persisting to SD card

---

## 💡 Resources

- [s-matyukevich/raspberry-pi-os](https://github.com/s-matyukevich/raspberry-pi-os)
- [bztsrc/raspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial)
- [BCM2837 ARM Peripherals Manual](https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf)
- [Raspberry Pi GPIO Pinout](https://pinout.xyz/)

---

## 📜 License

MIT License
