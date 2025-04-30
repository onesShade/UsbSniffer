# UsbSniffer

A comprehensive USB device detection and testing utility with real-time monitoring.

## Features

- 🕵️‍♂️ Detect all connected USB devices
- 📊 Display detailed device information (vendor, product, speed, power)
- 🧪 Perform read/write tests on storage devices
- 🖥️ Ncurses-based interactive interface

## Installation

### Dependencies
- Ncurses library
- GCC compiler
- Linux kernel

```bash
sudo dnf install gcc make ncurses-devel 

git clone https://github.com/onesShade/UsbSniffer.git
cd UsbSniffer
make
```

## Usage
```bash
cd bin
./USBSniffer
```
