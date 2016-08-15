/* midi.c -
     Midi receive buffer and processing routines

   (c) 2016 Ken Rossato
 */

#include "main.h"
#include "midi.h"
#include "keys.h"
#include "record.h"

/* Middle C is note 60 in MIDI,
   and note 40 (39 by 0-index) on
   an 88 key keyboard (and our array) */
#define MIDI_NOTE_OFFSET 21
#define MIDI_NOTE(x) ((x) - MIDI_NOTE_OFFSET)

#define MIDI_IDLE 0
#define MIDI_NOTE_ON 1
#define MIDI_NOTE_OFF 2
#define MIDI_PROGRAM_CHANGE 3
#define MIDI_CONTROL_CHANGE 4

volatile uint8_t midi_ring_buffer[64] __attribute__((aligned(64)));
volatile uint8_t midi_last_message = 0;
uint8_t midi_next_message = 0;

uint8_t midi_status = MIDI_NOTE_ON;
uint8_t midi_data_index = 0;
uint8_t midi_last_data;
uint8_t midi_sustain = 0;

void midi_program_change(uint8_t program) {
    switch(program) {
    case 0:
        banks[BANK_MIDI].shape = SHAPE_TRIANGLE;
        banks[BANK_MIDI].duty = 0x80;
        banks[BANK_MIDI].mode = MODE_VIBRATO;
        banks[BANK_MIDI].instrument = INSTRUMENT_DEFAULT;
        break;
    case 1:
        banks[BANK_MIDI].shape = SHAPE_SQUARE;
        banks[BANK_MIDI].duty = 0x80;
        banks[BANK_MIDI].mode = MODE_VIBRATO;
        banks[BANK_MIDI].instrument = INSTRUMENT_DEFAULT;
        break;
    case 4:
        banks[BANK_MIDI].shape = SHAPE_SQUARE;
        banks[BANK_MIDI].duty = 0x40;
        banks[BANK_MIDI].mode = MODE_VIBRATO;
        banks[BANK_MIDI].instrument = INSTRUMENT_DEFAULT;
        break;
    case 5:
        banks[BANK_MIDI].shape = SHAPE_SQUARE;
        banks[BANK_MIDI].duty = 0x40;
        banks[BANK_MIDI].mode = MODE_VIBRATO | MODE_ARPEGGIO;
        banks[BANK_MIDI].instrument = INSTRUMENT_DEFAULT;
        break;
    case 6:
        banks[BANK_MIDI].shape = SHAPE_TRIANGLE;
        banks[BANK_MIDI].duty = 0x80;
        banks[BANK_MIDI].mode = MODE_BURST | MODE_VIBRATO;
        banks[BANK_MIDI].instrument = INSTRUMENT_DEFAULT;
        break;
    case 7:
        banks[BANK_MIDI].shape = SHAPE_TRIANGLE;
        banks[BANK_MIDI].duty = 0x80;
        banks[BANK_MIDI].mode = MODE_PEDAL | MODE_VIBRATO;
        banks[BANK_MIDI].instrument = INSTRUMENT_DEFAULT;
        break;
    case 11:
        banks[BANK_MIDI].shape = SHAPE_DRUM;
        banks[BANK_MIDI].duty = 0x80;
        banks[BANK_MIDI].mode = 0;
        banks[BANK_MIDI].instrument = INSTRUMENT_DEFAULT;
        break;
    }
    notify_bank_mode_changed(BANK_MIDI);
}

void scan_midi() {
    uint8_t data = midi_ring_buffer[midi_next_message++];

    /* Status messages */
    if (data & 0x80) {
        midi_data_index = 0;
        switch(data & 0xF0) {
        case 0x90:
            midi_status = MIDI_NOTE_ON;
            break;
        case 0x80:
            midi_status = MIDI_NOTE_OFF;
            break;
        case 0xC0:
            midi_status = MIDI_PROGRAM_CHANGE;
            break;
        case 0xB0:
            midi_status = MIDI_CONTROL_CHANGE;
            break;
        default:
            midi_status = MIDI_IDLE;
        }
    }
    /* Data messages */
    else {
        midi_data_index += 1;
        switch (midi_status) {
        case MIDI_NOTE_ON:
            if (midi_data_index == 2) {
                if (midi_sustain && data) {
                    if (midi_sustain && midi_last_data == MIDI_NOTE_OFFSET)
                        key_octave_down();
                    else if (midi_sustain && midi_last_data == MIDI_NOTE_OFFSET + 87)
                        key_octave_up();
                    else
                        add_record_pitch_change(midi_last_data - MIDI_NOTE_OFFSET);
                }
                else {
                    if (data) {
                        start_note(BANK_MIDI, MIDI_NOTE(midi_last_data));
                        if (banks[BANK_MIDI].mode & MODE_PEDAL &&
                            midi_last_data >= (MIDI_NOTE_OFFSET+12))
                            start_note(BANK_MIDI, MIDI_NOTE(midi_last_data - 12));
                    }
                    else {
                        stop_note(BANK_MIDI, MIDI_NOTE(midi_last_data));
                        if (banks[BANK_MIDI].mode & MODE_PEDAL &&
                            midi_last_data >= (MIDI_NOTE_OFFSET+12))
                            stop_note(BANK_MIDI, MIDI_NOTE(midi_last_data - 12));
                    }
                }
                midi_data_index = 0;
            }
            break;
        case MIDI_NOTE_OFF:
            if (midi_data_index == 2) {
                stop_note(BANK_MIDI, MIDI_NOTE(midi_last_data));
                if (banks[BANK_MIDI].mode & MODE_PEDAL &&
                    midi_last_data >= (MIDI_NOTE_OFFSET+12))
                    stop_note(BANK_MIDI, MIDI_NOTE(midi_last_data - 12));
                midi_data_index = 0;
            }
            break;
        case MIDI_PROGRAM_CHANGE:
            midi_program_change(data);
            midi_data_index = 0;
            break;
        case MIDI_CONTROL_CHANGE:
            if (midi_data_index == 2) {
                if (midi_last_data == 0x40)
                    midi_sustain = data & 0x40; // <64 off, >=64 on
                midi_data_index = 0;
            }
            break;
        }
        midi_last_data = data;
    }
}
