# CHIP‑8 Emulator

A simple **CHIP‑8 emulator** written in C/C++ with build support using CMake and a Makefile.

CHIP‑8 is a **simple interpreted language** from the 1970s originally developed for 8‑bit computers like the COSMAC VIP and Telmac 1800. It’s a popular first‑time emulator project due to its simplicity and small opcode set.

## 🧠 What is CHIP‑8?

CHIP‑8 isn’t hardware — it’s a **virtual machine** that interprets programs designed for a 64×32 monochrome display, 4 KB memory, a small set of registers, timers, and a hex keypad.

## 📦 Project Structure

This repository includes:

.vscode/             → Editor configs  
build/               → Build output (if generated)  
chip8‑roms/          → Example CHIP‑8 ROM files  
src/                 → Emulator source code  
CMakeLists.txt       → CMake build config  
Makefile             → Make build script  

## 🛠️ Features

✔️ Implements core CHIP‑8 CPU loop and instruction set  
✔️ Loads and executes `.ch8` ROM files  
✔️ Display framebuffer for 64×32 graphics  
✔️ (Optional) Keyboard and input mapping  
✔️ Cross‑platform build support via CMake / Make  

## 📥 Requirements

To build & run this emulator, ensure you have:

- **C/C++ Compiler** (GCC, Clang, MSVC)
- **CMake** (if using the CMake build)
- Make (if using Makefile)
- (Optional) SDL2 or other window/input libs if GUI support is included

## 🧩 Build & Run

### Using **CMake**

```sh
git clone https://github.com/Aditya‑Raval/chip8.git
cd chip8
mkdir build && cd build
cmake ..
cmake --build .
```

### Using **Makefile**

```sh
make
```

## ▶️ Running a ROM

After building, run:

```sh
./chip8 <path/to/rom.ch8>
```

Example:

```sh
./chip8 chip8‑roms/Pong.ch8
```

## ⌨️ Controls

CHIP‑8 uses a **16‑key hex keypad** (0x0–0xF). Typical keyboard mapping:

1 2 3 4    →   1 2 3 C  
Q W E R    →   4 5 6 D  
A S D F    →   7 8 9 E  
Z X C V    →   A 0 B F  

## ❓ What This Project Teaches

Writing a CHIP‑8 emulator teaches:

✔ CPU fetch‑decode‑execute loops  
✔ Memory layout and stack  
✔ Opcode decoding & bitwise ops  
✔ Graphics rendering & input handling  

## 📄 License

*(Add your chosen license here, e.g., MIT / GPL)*

