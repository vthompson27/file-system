# 🧠 Raspberry Pi 3 File System Development

This project is a bare-metal learning environment for developing a simple file system from scratch on the Raspberry Pi 3.

## 🔧 Features

- Runs without an operating system (bare-metal)
- Direct interaction with hardware
- UART serial output for debugging
- Basic memory and I/O routines
- Simple custom file system planned (FAT-like or log-structured)

## 🧱 Project Structure

```
.
├── Makefile           # Build system using arm-none-eabi toolchain
├── src/               # Source files (C and assembly)
│   ├── kernel.c       # Entry point
│   ├── utils.S        # Basic hardware routines (put32, get32, delay)
│   ├── uart.c         # UART output
│   ├── fs.c           # File system logic (in progress)
│   └── ...
├── include/           # Header files
│   └── peripherals/   # GPIO, UART, etc.
├── build/             # Build output directory
├── kernel.img         # Final image to boot on the Pi
├── config.txt         # Boot configuration for Pi firmware
└── README.md
```

## 🚀 Getting Started

### 1. Requirements

- Raspberry Pi 3
- Cross compiler: `arm-none-eabi-gcc`
- USB-to-TTL Serial cable
- External LED (optional for debugging)
- SD card (FAT32, MBR partitioned)
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
enable_uart=1
arm_64bit=0
disable_commandline_tags=1
kernel_old=1
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
- [ ] Disk block abstraction
- [ ] File system layout (in-memory prototype)
- [ ] Directory structure and file metadata
- [ ] Read/write file operations
- [ ] Persisting to SD card

---

## 🧪 Testing

We will use UART for most logging. Later stages will include tests for:

- Block allocation
- File integrity
- Directory resolution

---

## 💡 Resources

- [s-matyukevich/raspberry-pi-os](https://github.com/s-matyukevich/raspberry-pi-os)
- [BCM2837 ARM Peripherals Manual](https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf)
- [Raspberry Pi GPIO Pinout](https://pinout.xyz/)

---

## 📜 License

MIT License
