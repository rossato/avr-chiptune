Arduino chiptune synthesizer
============================

This is my 8-bit synthesizer project for the Arduino UNO.  It'll probably work with minimal modification on any 8-bit AVR with at least 2k of RAM at 16MHz.  I compile with avr-gcc and upload with a SPI programmer, but it doesn't take up a ton of flash space so you should in theory be able to keep Optiboot if you tweak the makefile.

Features:
*  8 simultaneous independent channels at 44.1kHz
*  Input over MIDI and 7x8 key matrix with separate assignable voices
*  Square wave, triangle wave, and noise
*  Square wave duty cycle adjustable to 50%, 25%, or 12.5%
*  Attack-decay-sustain envelopes
*  Vibrato, arpeggio, and echo effects
*  Preprogrammed percussion using sliding pitches and shaped noise
*  "Burst note" setting that plays hi-hat with notes
*  8-beat record and playback (work in progress)

This project is inspired by Linus Åkesson's incredible [Chipophone](http://linusakesson.net/chipophone/index.php).  My source code is written from scratch and is hopefully a little more reusable (I aim for MIDI and Arduino compatibility).  It uses the same MCP49x1 DAC for output and can be wired up with a few external components:

For the audio output:

*  MCP4921 DAC (can substitute 4911 or 4901)
*  470uF electrolytic capacitor to block DC on the output
*  100nF electorlytic capacitor to filter out noise on the volume knob
*  10kOhm linear potentiometer to select the Vref level
*  56kOhm volume limiting resistor
*  Audio jack

For MIDI input:

*  6N138 optoisolator
*  1N4148 diode
*  220 and 470 ohm resistors
*  MIDI socket

For key-matrix input:

*  74HC164 or 74HC595 shift register
