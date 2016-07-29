TARGET = chiptune

MCU   = atmega328p
F_CPU = 16000000UL
BAUD  = 31250UL

CC      = avr-gcc
OBJCOPY = avr-objcopy
AVRSIZE = avr-size
AVRDUDE = avrdude

AVRDUDE_FLAGS = -C avrisp -P /dev/ttyACM0 -b 19200

CSOURCES=$(wildcard *.c)
SSOURCES=$(wildcard *.S)
SOURCES=$(CSOURCES) $(SSOURCES)
OBJECTS=$(CSOURCES:.c=.o) $(SSOURCES:.S=.o)
HEADERS=$(wildcard *.h)

NOISE_SHIFT_REGISTERS = -ffixed-r2 -ffixed-r3

CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I.

CFLAGS  = -O2 -std=gnu99 -Wall -Wimplicit-function-declaration
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

.PHONY: all size clean flash

all: $(TARGET).hex

size: $(TARGET).elf
	$(AVRSIZE) -C --mcu=$(MCU) $(TARGET).elf

clean:
	rm -f *.elf *.hex *.o

flash: $(TARGET).hex
	$(AVRDUDE) -p $(MCU) $(AVRDUDE_FLAGS) -U flash:w:$<
