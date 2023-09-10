PLATFORM := $(shell uname)

ifeq ($(PLATFORM), Darwin) 
CC := i386-elf-gcc
LD := i386-elf-ld
OBJCOPY := gobjcopy
else
CC := gcc
LD := ld
OBJCOPY := objcopy
endif

QEMU_GDB_TIMEOUT ?= 10 # num. seconds to wait for qemu to start OS

export CC
export LD
export OBJCOPY

export CFLAGS := -Wall -Werror -g3 -Os -Wl,--oformat=binary -no-pie -m32 -s -falign-functions=4 -ffreestanding -fno-asynchronous-unwind-tables
export LFLAGS := -melf_i386 --build-id=none

ASM_BOOT_SECT_SOURCE := ./src/boot/boot_sect.asm
ASM_OS_ENTRY_SOURCE := ./src/boot/os_entry.asm

BOOT_OBJ := boot.o
OS_BIN := mOS.bin

C_FILES = $(shell find ./ -name '*.[ch]')

OBJ_NAMES := src/os/main.o src/os/test.o os_entry.o src/os/paging.o \
	src/os/hard/idt.o src/os/hard/except.o src/os/hard/pic.o \
	src/lib/device/serial.o src/lib/container/ring_buffer.o \
	src/lib/stdlib/stdio.o src/lib/stdlib/stdlib.o src/lib/stdlib/string.o \
  src/lib/pit/pit.o src/lib/video/VGA_text.o


.PHONY: clean qemu test

$(OS_BIN): $(OBJ_NAMES) $(BOOT_OBJ)
	$(LD) $(LFLAGS) -T link.ld $(OBJ_NAMES) -o mOS.elf
	$(OBJCOPY) -O binary mOS.elf intermediate.bin
	cat $(BOOT_OBJ) intermediate.bin > $(OS_BIN)

$(BOOT_OBJ): $(ASM_BOOT_SECT_SOURCE)
	nasm $^ -f bin -o $@

os_entry.o: $(ASM_OS_ENTRY_SOURCE)
	nasm $^ -f elf32 -g -o $@

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS) -I./src/lib/

%.o: %.asm
	nasm $^ -f elf32 -g -o $@

qemu: $(OS_BIN)
	qemu-system-i386 -boot c -drive format=raw,file=$^ -no-reboot -no-shutdown

qemu-gdb: $(OS_BIN)
	qemu-system-i386 -s -S -boot c -drive format=raw,file=$^ -no-reboot -no-shutdown &
	gdb mOS.elf \
		-ex 'set remotetimeout $(QEMU_GDB_TIMEOUT)' \
		-ex 'target remote localhost:1234' \
		-ex 'break os_main' \
		-ex 'continue'

qemu-gdb-boot: $(OS_BIN)
	qemu-system-i386 -s -S -boot c -drive format=raw,file=$^ -no-reboot -no-shutdown &
	gdb -ix gdb_init_real_mode.txt mOS.elf \
		-ex 'set remotetimeout $(QEMU_GDB_TIMEOUT)' \
		-ex 'target remote localhost:1234' \
		-ex 'break *0x7c00' \
		-ex 'continue'

test: $(OS_BIN)
	cd tests && $(MAKE) clean
	cd tests && $(MAKE) test

format: $(C_FILES)
	for file in $?; do \
		clang-format -i $$file && echo "formatted $$file"; \
	done

clean:
	rm -f mOS
	rm -f *.o $(OBJ_NAMES) *.bin *.elf *.~ src/*~ src/boot/*~ docs/*~
	cd tests && $(MAKE) clean
