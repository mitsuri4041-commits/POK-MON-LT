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
          $(ARCH) -D__3DS__

LDFLAGS := -specs=3dsx.specs -g $(ARCH)

LIBS := -lcitro3d -lctru -lm

VPATH := $(SOURCES)

CFILES := $(wildcard $(SOURCES)/*.c)
OFILES := $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(CFILES))

SHADER := $(BUILD)/world.v.shbin
SHADER_HEADER := $(BUILD)/world_v_shbin.h

.PHONY: all clean

all: $(TARGET).3dsx

$(BUILD):
	mkdir -p $(BUILD)

$(SHADER): $(SOURCES)/world.v.pica | $(BUILD)
	picasso -o $@ $<

$(SHADER_HEADER): $(SHADER)
	bin2c $< $@ world_v_shbin

$(BUILD)/main.o: $(SOURCES)/main.c $(SHADER_HEADER) | $(BUILD)
	$(CC) $(CFLAGS) -I$(BUILD) -c $< -o $@

$(TARGET).elf: $(BUILD)/main.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).3dsx
