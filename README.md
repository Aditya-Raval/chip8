# CHIP‑8 Emulator

A simple **CHIP‑8 emulator** written in C with build support using CMake and a Makefile.

CHIP‑8 is a **simple interpreted language** from the 1970s originally developed for 8‑bit computers like the COSMAC VIP and Telmac 1800. It’s a popular first‑time emulator project due to its simplicity and small opcode set.

## Requirements

To build & run this emulator, ensure you have:

- **C/C++ Compiler** (GCC, Clang, MSVC)
- **CMake** (if using the CMake build)
- Make (if using Makefile)
- (Optional) SDL2 or other window/input libs if GUI support is included

## Build & Run

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

## Running a ROM

After building, run:

```sh
./chip8 <path/to/rom.ch8>
```

Example:

```sh
./chip8 chip8‑roms/Pong.ch8
```

## Controls

CHIP‑8 uses a **16‑key hex keypad** (0x0–0xF):

1 2 3 4    →   1 2 3 C  
Q W E R    →   4 5 6 D  
A S D F    →   7 8 9 E  
Z X C V    →   A 0 B F  


