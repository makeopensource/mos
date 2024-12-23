C_FILES := $(shell find ./src/ -name '*.c')
BIN_NAMES := $(C_FILES:%.c=%.bin)
OBJ_NAMES := $(C_FILES:%.c=%.o) 

MOS_ELF := ./../mOS.elf

VENV = .venv
VENV_PY = $(VENV)/bin/python

.PHONY: clean test

all: $(MOS_ELF) $(BIN_NAMES) venv_setup

venv_setup:
	python3 -m venv $(VENV)
	$(VENV_PY) -m pip install -r requirements.txt

%.bin: %.o test_entry.o
	$(LD) -o $@.elf $(LFLAGS) -T link.ld $^ --just-symbols=$(MOS_ELF)
	$(OBJCOPY) -O binary $@.elf $@
	rm $@.elf

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS) -I./src -I./../src/lib/ -I./../src/lib/stdlib/

test_entry.o: test_entry.asm
	nasm $^ -f elf32 -o $@

$(MOS_ELF):
	cd ./../ && $(MAKE)

test: all
ifdef TESTS
	$(VENV_PY) test.py $(TESTS)
else
	$(VENV_PY) test.py
endif

clean:
	rm -f test_entry.o
	rm -f $(BIN_NAMES) $(OBJ_NAMES) 
	rm -rf *.got
