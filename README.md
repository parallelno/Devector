# Devector

## Overview

Devector is a multi-platform emulator of a Soviet personal computer Vector06c. It is designed to simplify the development process and speed up the work. Currently, it is in the early stages of development, so please use it on your own risk.

## Features

- A multi-platform precise emulator of a Soviet personal computer Vector06c
- Debugging functionality
- FDD support
- Up to 8 RAM Disk support
- AY & bipper & 3-channel timer support
- Recording a playback with options to store, load, and play it

## Usage

To run the emulator: 
`./Devector.exe` <-settingsPath settings.json> <-path rom_fdd_rec_file>

## Build

ImGui frontend:

On Windows

It requires VS 2019+ c++ development environment installed
1. Install MSYS2. https://www.msys2.org/
2. Run MSYS2 terminal
3. In this terminal, install the MinGW-w64 toolchain: 

pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain. 
Enter Y when prompted

4. Update Path environment variable adding following paths:

C:\msys64\ucrt64\bin

5. mkdir build
6. cd build
7. cmake --build build --target all


On Linux
1. sudo apt update
2. sudo apt-get install build-essential git gcc-13 g++-13 make cmake libgl1-mesa-dev xorg-dev ninja-build libgtk-3-dev libx11-dev
3. mkdir build
3. cd build
4. cmake -G Ninja ..
5. ninja
or 
4. cmake ..
5. cmake --build .

WPF frontend:
It requires VS 2019+ c++ development environment installed
1. open DevectorWPF.sln VS Studio solution
2. install NuGet package:
    1. Tools -> NuGet Package Manager -> Package Manager Console
    2. Install-Package ModernWpfUI
3. build

## FAQ

### Scripts Window

**Scripts** are short code excerpts written in **Lua**. They are executed on every CPU command and can:

- Access memory and registers
- Interact with hardware state
- Interrupt execution
- Draw primitives on the screen

---

### How can I add a script?

You can add a script in the **Debug Data** window under the **Scripts** tab.

---

### What functionality do scripts support?

#### CPU State Access

```lua
GetCC()      -- Get CPU cycles
GetPC()      -- Get Program Counter
GetSP()      -- Get Stack Pointer
GetPSW()     -- Get Program Status Word
GetBC()      -- Get BC register pair
GetDE()      -- Get DE register pair
GetHL()      -- Get HL register pair
GetA()       -- Get A register
GetF()       -- Get Flags register
GetB(), GetC(), GetD(), GetE(), GetH(), GetL() -- Individual registers

GetFlagS()   -- Sign flag
GetFlagZ()   -- Zero flag
GetFlagAC()  -- Auxiliary Carry flag
GetFlagP()   -- Parity flag
GetFlagC()   -- Carry flag
GetINTE()    -- Interrupt Enable
GetIFF()     -- Interrupt Flip-Flop state
GetHLTA()    -- Temporary HL register
GetMachineCycles() -- Current machine cycles

GetOpcode()  -- Current opcode
```

#### Memory Access

```lua
GetByteGlobal(addr) -- Read a byte at a global memory address
```

#### Execution Control

```lua
Break() -- Stop CPU execution
```

#### UI Drawing

- **DrawText**

```lua
DrawText(id, "TEXT", x, y, <color=0xFFFFFFFF>, <vectorScreenCoords=true>)
```

- `color`: RGBA (e.g., 0xFFFFFFFF)

- Negative `x/y`: origin from right/bottom

- `vectorScreenCoords`:

  - `true`: Vector06C screen coordinates (active screen only)
  - `false`: Full display window

- **DrawRect**

```lua
DrawRect(id, x, y, width, height, <color=0xFFFFFFFF>, <vectorScreenCoords=true>)
```

- **DrawRectFilled**

```lua
DrawRectFilled(id, x, y, width, height, <color=0xFFFFFFFF>, <vectorScreenCoords=true>)
```

---

### Script Examples

#### 1. Check if BC is overwritten in a critical stack section

```lua
local sp = GetSP()
if sp > 0x7FBE and sp < 0x7FFE then return end
local opcode = GetOpcode()
if opcode > 0x4F then return end
if opcode > 0x0E and opcode < 0x40 then return end

local opcodesToSkip = {
    [0x00] = true,
    [0x02] = true,
    [0x07] = true,
    [0x08] = true,
    [0x09] = true,
    [0x0A] = true,
}
if opcodesToSkip[opcode] then return end
if not GetINTE() then return end
if GetByteGlobal(0x38) ~= 0xC3 or GetByteGlobal(0x39) ~= 0x78 or GetByteGlobal(0x3A) ~= 0x05 then return end

print("Warning! Overwriting BC in a stack critical section")
print(string.format("PC = %04X", GetPC()))
print(string.format("Opcode = %02X", opcode))
Break()
```

#### 2. Draw FPS Counter

```lua
local pc = GetPC()
if pc ~= 0x5CB then return end

local fpsL = GetByteGlobal(0x5CC)
local fpsH = GetByteGlobal(0x5CD)
local fps = fpsH * 256 + fpsL

DrawText(0, string.format("FPS %d", fps), 0, 0, 0xFFFFFFFF, false)
```

#### 3. Draw a Rectangle

```lua
local pc = GetPC()
if pc ~= 0x5CB then return end

DrawRect(1, 0, 0, 16, 16, 0xFFFFFF80)
```

---

### Tips

- Each function like `GetSP()` or `GetHL()` is **expensive** to call repeatedly.
- Store results in local variables:

```lua
local sp = GetSP()
```

## Contributing

Contributions are welcome! If you find a bug or have an idea for a feature, please create an issue or submit a pull request.

## License

Devector is licensed under the MIT License. See the LICENSE file for more information.

## Acknowledgements

Special thanks to the following people for their contributions:

- [Svofski](https://github.com/svofski)
- [Viktor Pykhonin](https://github.com/vpyk/)
- [Yuri Larin](https://github.com/ImproverX)
- [zx-pk.ru comunity](https://zx-pk.ru/)