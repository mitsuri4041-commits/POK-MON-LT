ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set)
endif

include $(DEVKITPRO)/devkitARM/3ds_rules

TARGET := pokemon_world
BUILD := build
SOURCES := source
DATA := data
INCLUDES := include

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -O2 -mword-relocations \
          -ffunction-sections -fdata-sections \
          $(ARCH) -D__3DS__ \
          -I$(DEVKITPRO)/libctru/include \
          -I$(DEVKITPRO)/citro3d/include

LDFLAGS := -specs=3dsx.specs -g $(ARCH) \
           -L$(DEVKITPRO)/libctru/lib \
           -L$(DEVKITPRO)/citro3d/lib

LIBS := -lcitro3d -lctru -lm

CFILES := $(wildcard $(SOURCES)/*.c)
OFILES := $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(CFILES))

.PHONY: all clean

all: $(TARGET).3dsx

$(BUILD)/%.o: $(SOURCES)/%.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OFILES)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).3dsx
