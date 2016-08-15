TARGET = chiptune

MCU   = atmega328p
F_CPU = 16000000UL
BAUD  = 31250UL

CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRSIZE = avr-size
AVRDUDE = avrdude

AVRDUDE_FLAGS = -c avrisp -p m328p -P /dev/ttyACM0 -b 19200
#AVRDUDE_FLAGS = -c arduino -p m328p -P /dev/ttyACM0 -b 115200

CSOURCES=$(wildcard *.c)
SSOURCES=$(wildcard *.S)
SOURCES=$(CSOURCES) $(SSOURCES)
OBJECTS=$(CSOURCES:.c=.o) $(SSOURCES:.S=.o)
HEADERS=$(wildcard *.h)

NOISE_SHIFT_REGISTERS = -ffixed-r2 -ffixed-r3

CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I.

CFLAGS  = -O2 -g -std=gnu99 -Wall -Wimplicit-function-declaration
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += $(NOISE_SHIFT_REGISTERS)

LDFLAGS  = -Wl,-Map,$(TARGET).map
LDFLAGS += -Wl,--gc-sections

ARCH = -mmcu=$(MCU)

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ARCH) -c -o $@ $<

.S.o: $(HEADERS) Makefile
	$(CC) $(CPPFLAGS) $(ARCH) -c -o $@ $<;

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(ARCH) $^ -o $@

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.lst: %.elf
	$(OBJDUMP) -S $< > $@

.PHONY: all size clean flash

all: $(TARGET).hex

size: $(TARGET).elf
	$(AVRSIZE) -C --mcu=$(MCU) $(TARGET).elf

clean:
	rm -f *.elf *.hex *.o

flash: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<
