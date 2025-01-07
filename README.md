# Devector

## Overview

Devector is a multi-platform emulator of a Soviet personal computer Vector06c. It is designed to simplify the development process and speed up the work. Currently, it is in the early stages of development, so please use it on your own risk.

## Features

- A multi-platform precise emulator of a Soviet personal computer Vector06c
- Debugging functionality
- FDD support
- Up to 8 Ram-Disk support
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
