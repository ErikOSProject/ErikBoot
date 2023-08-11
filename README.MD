# ErikBoot

ErikBoot is a UEFI-compatible bootloader for ErikOS.

## Features

* Loads an ErikKernel executable in ELF format
* Loads a framebuffer console font in PSF1 format
* Loads an initial ramdisk in USTAR format

## Usage

You can build ErikBoot with Make: 

```bash
make all
```

Building the code has only been tested on `clang`. 

The resulting file, `erikboot.efi`, can be booted with a UEFI compliant firmware.

To clean the build tree: 

```bash
make clean
```

## License

ErikBoot is licensed under [BSD-2-Clause](COPYING) license.

The files under `efi` subdirectory are a part of the EFI Application Development Toolkit released by Intel Corporation under [BSD-2-Clause](efi/COPYING) license.
