C_FILES := $(shell find ./src/ -name '*.c')
BIN_NAMES := $(C_FILES:%.c=%.bin)
OBJ_NAMES := $(C_FILES:%.c=%.o) 

MOS_ELF := ./../mOS.elf

.PHONY: clean test

all: $(MOS_ELF) $(BIN_NAMES)

%.bin: %.o test_entry.o
	$(LD) -o $@.elf $(LFLAGS) -T link.ld $^ --just-symbols=$(MOS_ELF)
	$(OBJCOPY) -O binary $@.elf $@
	rm $@.elf

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS) -I./../src/lib/

test_entry.o: test_entry.asm
	nasm $^ -f elf32 -o $@

$(MOS_ELF):
	cd ./../ && $(MAKE)

test: all
ifdef TESTS
	python3 test.py $(TESTS)
else
	python3 test.py
endif

clean:
	rm -f test_entry.o
	rm -f $(BIN_NAMES) $(OBJ_NAMES) 
	rm -rf *.got
