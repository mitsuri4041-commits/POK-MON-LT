
ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set)
endif

include $(DEVKITPRO)/devkitARM/3ds_rules

TARGET := pokemon_world
BUILD := build
SOURCES := source

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -O2 -mword-relocations \
          -ffunction-sections -fdata-sections \
          $(ARCH) -D__3DS__ \
          -I$(DEVKITPRO)/libctru/include \
          -I$(DEVKITPRO)/citro3d/include \
          -I$(BUILD)

LDFLAGS := -specs=3dsx.specs -g $(ARCH) \
           -L$(DEVKITPRO)/libctru/lib \
           -L$(DEVKITPRO)/citro3d/lib

LIBS := -lcitro3d -lctru -lm

SHADER := $(BUILD)/world.v.shbin
SHADER_HEADER := $(BUILD)/world_v_shbin.h
MAIN_OBJ := $(BUILD)/main.o

.PHONY: all clean

all: $(TARGET).3dsx

$(BUILD):
	mkdir -p $(BUILD)

$(SHADER): $(SOURCES)/world.v.pica | $(BUILD)
	picasso -o $@ $<

$(SHADER_HEADER): $(SHADER)
	python3 -c 'import sys,struct; d=open(sys.argv[1],"rb").read(); n=len(d); d+=b"\0"*((-n)%4); w=struct.unpack("<%dI"%(len(d)//4),d); f=open(sys.argv[2],"w"); f.write("#pragma once\n#include <stdint.h>\nstatic const uint32_t world_v_shbin[] __attribute__((aligned(4))) = {"+",".join("0x%08x"%x for x in w)+"};\nstatic const unsigned int world_v_shbin_size = "+str(n)+";\n")' $< $@

$(MAIN_OBJ): $(SOURCES)/main.c $(SHADER_HEADER) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(MAIN_OBJ)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).3dsx
