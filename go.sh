#!/bin/sh
rm krackrainbow.elf krackrainbow.hex
avr-gcc -DSLAVE_ADDRESS=$1 -O2 -mmcu=atmega328p krackrainbow.c data.c -o krackrainbow.elf
avr-objcopy -j .text -j .data -O ihex krackrainbow.elf krackrainbow.hex
avrdude -c avrisp -F -p atmega328p -P /dev/ttyACM0 -b 19200 -Uflash:w:krackrainbow.hex:i
