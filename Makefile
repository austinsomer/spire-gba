PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

TITLE   := SPIRE
BUILD   := build
ROM     := $(BUILD)/spire.gba
ELF     := $(BUILD)/spire.elf

SRC_C   := $(wildcard src/*.c)
SRC_S   := src/crt0.s
OBJ     := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC_C)) $(BUILD)/crt0.o

ARCH    := -mcpu=arm7tdmi -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -Wall -Wextra -Wno-unused-parameter -fno-strict-aliasing \
           -ffunction-sections -fdata-sections -Isrc $(EXTRA)
LDFLAGS := $(ARCH) -nostartfiles -nostdlib -T src/gba.ld -Wl,--gc-sections -lgcc

all: $(ROM)

$(BUILD)/%.o: src/%.c src/gba.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/crt0.o: src/crt0.s | $(BUILD)
	$(CC) $(ARCH) -c $< -o $@

$(ELF): $(OBJ) src/gba.ld
	$(CC) $(OBJ) $(LDFLAGS) -o $@

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	python3 tools/gbafix.py $@ $(TITLE)

$(BUILD):
	mkdir -p $(BUILD)

run: $(ROM)
	open -a mGBA $(ROM)

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
