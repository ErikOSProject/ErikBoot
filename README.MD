# ErikBoot

ErikBoot is a UEFI-compatible bootloader for ErikOS.

## Usage

You can build ErikBoot with CMake: 

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../efi-x64.cmake ..
make
```

The CMake variable `CMAKE_TOOLCHAIN_FILE` needs to point to a toolchain specification. This repository includes efi-aarch64.cmake and efi-x64.cmake that use clang to cross-compile for their respective architectures.

The resulting file, `ErikBoot.efi`, can be booted with a UEFI compliant firmware.

## License

ErikBoot is licensed under [BSD-2-Clause](COPYING) license.

The headers under `include/UEFI` subdirectory are a part of the EDK II Project released under [BSD-2-Clause-Patent](include/UEFI/COPYING) license.
