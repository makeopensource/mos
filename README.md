# MakeOpenSource Operating System

The major goal of this project is to recreate some of the functionality from a DOS-like operating system.

## Objectives (in no particular order)

* BIOS boot
* Basic memory initialization
* PS2 keyboard support (scan code conversion)
* VGA 40/80 column text mode console
* Commands (programs)
  * cd
  * pwd
  * ls
  * mkdir
  * rm
  * cp
  * clear (C-L)
  * man
  * vi
  * ed

## Contributing

For contributing _fork_ this repository and then make your changes in the form of Pull Requests to the main repository

## Building and running

### Dependencies

* make
* qemu (specifically `qemu-system-x86`) on debian based systems you can install this with the apt package manager
* gcc
* nasm
* clang-format
* WSL (Windows only)

#### For MacOS

Use homebrew to install the dependencies using these commands:

```shell
brew install x86_64-elf-binutils x86_64-elf-gcc nasm binutils qemu
```

If you are on Apple Silicon, you will need to use Rosetta along with installing x86_64 libraries. This can be done by prefixing the previous commands with `arch -x86_64`.
  
You will need to ensure that these programs are on your PATH or `make` will not find them.

### Building

* just run `make` in the root directory of the project

### Running

* run `make qemu` in the root directory of the project

### Debugging

* run `make qemu-gdb` in the root directory of the project
* run `make qemu-gdb-boot` in the root directory for debugging the bootloader

### Running in DEBUG mode

* run `make qemu DEBUG=true` in the root directory of the project

### Formatting

* run `make format` in the root directory of the project to format the code
   according to the formatter in .clang-format

## General behavior

boot and immediately drop to a prompt.
programs can be executed from the prompt.
send signals by keyboard to do ✨stuff✨.

## Resources

* [qemu](https://www.qemu.org/)
* [osdev wiki](https://wiki.osdev.org/Main_Page)
* [osdev linker scripts](https://wiki.osdev.org/Linker_Scripts)

## FAQ

* When you run `make` and you get an error along the lines of 
  `/usr/bin/ld: cannot find crt1.o: No such file or directory`
  you likely only have the gcc for your current architecture that is 64bit. You need the 32bit support files. For that you can install them on debian based machines with `sudo apt install gcc-multilib`
