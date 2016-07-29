/* midi.h -
     Midi receive buffer and processing routines

   (c) 2016 Ken Rossato
 */

#ifndef MIDI_H
#define MIDI_H

extern uint8_t midi_next_message;
extern volatile uint8_t midi_last_message;

void scan_midi();

#endif
