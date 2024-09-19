# Devector

## Overview

Devector is a cross-platform emulator of a Soviet personal computer Vector06c. It is designed to simplify the development process and speed up the work. Currently, it is in the early stages of development, so please use it on your own risk.

## Features

- A cross-platform precise emulator of a Soviet personal computer Vector06c
- Debugging functionality
- FDD support
- Up to 8 Ram-Disk support
- AY & bipper & 3-channel timer support
- Recording a playback with options to store, load, and play it

## Usage

To use Devector, follow these steps:

1. Run the emulator: `./Devector.exe`

## Build

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
