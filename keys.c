/* keys.c -
     Routines to scan the piano key matrix

   (c) 2016 Ken Rossato
 */

#include "main.h"
#include "keys.h"

#define KEY_SIZE 49
uint8_t key_octave_transpose = 15;

void key_octave_up() {
    if (key_octave_transpose < 27)
        key_octave_transpose += 12;
}

void key_octave_down() {
    if (key_octave_transpose > 3)
        key_octave_transpose -= 12;
}

void scan_keys() {
    PORTB &= ~(_BV(PB0) | _BV(PB1));
    PORTB |= _BV(PB0);
    PORTB &= ~_BV(PB0);
    PORTB |= _BV(PB1);

#ifdef SHIFT_74HC595
    // I use the simpler 74HC164, but you can use a 595 by tying the two
    //  clock inputs together.  You'll need this extra clock pulse to load the first bit.
    PORTB |= _BV(PB0);
    PORTB &= ~_BV(PB0);
#endif
    
    uint8_t col;
    for (col = 11; col > 3; --col) {
        uint8_t rows = PIND | 0x01;
        PORTB |= _BV(PB0);
        PORTB &= ~_BV(PB0);

        uint8_t note;

        uint8_t col_start = (col & 7) + key_octave_transpose;
        uint8_t col_end = KEY_SIZE + key_octave_transpose;
        if (rows != 0xFF) {
            for (note = col_start; note < col_end; note += 8) {
                if (!(rows & 0x80))
                    start_note(note);
                else
                    stop_note(note);
                rows = rows << 1;
            }
        }
        else {
            for (note = col_start; note < col_end; note += 8) {
                stop_note(note);
            }
        }
    }
}
