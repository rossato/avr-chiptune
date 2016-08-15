/* interface.c -
     Routines for managing the connected LEDs and buttons

   (c) 2016 Ken Rossato
 */

#include "main.h"
#include "interface.h"
#include "record.h"

#define BUTTON_TOP PC1
#define BUTTON_RIGHT PC2
#define BUTTON_LEFT PC3

uint8_t top_debounce = 0;
uint8_t right_debounce = 0;
uint8_t left_debounce = 0;
uint8_t voice = 0, effect = 0;

void scan_interface() {
    uint8_t i;
    if (!(PINC & _BV(BUTTON_TOP)) && !top_debounce) {
        top_debounce = 12;
        switch (record_mode) {
        case RECORD_MODE_NONE:
            set_cpu_bank_mode(BANK_KEYS);
            record_mode = RECORD_MODE_RECORDING;
            reset_recording();
            break;
        case RECORD_MODE_RECORDING:
            record_mode = RECORD_MODE_PLAYBACK;
            PORTC &= ~_BV(PC4);
            break;
        case RECORD_MODE_PLAYBACK:
            for (i = 0; i < TOTAL_NOTES; ++i) {
                stop_note(BANK_CPU, i);
            }
            record_mode = RECORD_MODE_NONE;
            PORTC |= _BV(PC4);
            break;
        }
    }
    if (!(PINC & _BV(BUTTON_LEFT)) && !left_debounce) {
        left_debounce = 12;
        if (record_mode == RECORD_MODE_PLAYBACK)
            PORTC |= _BV(PC4);
        else
            PORTC &= ~_BV(PC4);

        voice = (voice + 1)%5;
        switch (voice) {
        case 0:
            banks[BANK_KEYS].shape = SHAPE_SQUARE;
            banks[BANK_KEYS].duty = 0x80;
            banks[BANK_KEYS].instrument = INSTRUMENT_DEFAULT;
            break;
        case 1:
            banks[BANK_KEYS].shape = SHAPE_SQUARE;
            banks[BANK_KEYS].duty = 0x40;
            banks[BANK_KEYS].instrument = INSTRUMENT_DEFAULT;
            break;
        case 2:
            banks[BANK_KEYS].shape = SHAPE_SQUARE;
            banks[BANK_KEYS].duty = 0x20;
            banks[BANK_KEYS].instrument = INSTRUMENT_DEFAULT;
            break;
        case 3:
            banks[BANK_KEYS].shape = SHAPE_TRIANGLE;
            banks[BANK_KEYS].duty = 0x80;
            banks[BANK_KEYS].instrument = INSTRUMENT_DEFAULT;
            break;
        case 4:
            banks[BANK_KEYS].shape = SHAPE_DRUM;
            banks[BANK_KEYS].duty = 0x80;
            banks[BANK_KEYS].instrument = INSTRUMENT_DEFAULT;
            break;
        }
        notify_bank_mode_changed(BANK_KEYS);
    }
    if (!(PINC & _BV(BUTTON_RIGHT)) & !right_debounce) {
        right_debounce = 12;
        PORTC &= ~_BV(PC5);

        effect = (effect + 1)%4;
        switch (effect) {
        case 0:
            banks[BANK_KEYS].mode = MODE_VIBRATO;
            break;
        case 1:
            banks[BANK_KEYS].mode = MODE_BURST | MODE_VIBRATO;
            break;
        case 2:
            banks[BANK_KEYS].mode = MODE_ECHO | MODE_VIBRATO;
            break;
        case 3:
            banks[BANK_KEYS].mode = MODE_ARPEGGIO | MODE_VIBRATO;
            break;
        }
        notify_bank_mode_changed(BANK_KEYS);
    }
}

void update_interface() {
    if (top_debounce) --top_debounce;
    if (left_debounce) {
        if (--left_debounce == 0) {
            if (record_mode == RECORD_MODE_PLAYBACK)
                PORTC &= ~_BV(PC4);
            else
                PORTC |= _BV(PC4);
        }
    }
    if (right_debounce) {
        if (--right_debounce == 0) {
            PORTC |= _BV(PC5);
        }
    }
}
