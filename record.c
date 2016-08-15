/* record.c -
     Sequence recording and playback routines

   (c) 2016 Ken Rossato
 */

#include "main.h"
#include "record.h"
#include "drum.h"

#define RECORD_BEATS 16
#define RECORD_EVENTS 8

#define RECORD_OFF_BASE 88
#define RECORD_DRUM_BASE 176

#define RECORDED_KEY(key) ((key) + record_base_pitch[record_pitch_counter] - record_base_pitch[0])

uint8_t record_mode = RECORD_MODE_NONE;
uint8_t ticks_per_beat = 10;
uint8_t tick = 0, beat = 0, event = 0;

uint8_t record_matrix[RECORD_BEATS][RECORD_EVENTS] = {
    [0 ... RECORD_BEATS-1] = {
        [0 ... RECORD_EVENTS-1] = NO_VALUE
    }
};
uint8_t record_matrix_next[RECORD_EVENTS] = {
    [0 ... RECORD_EVENTS-1] = NO_VALUE
};

#define NPITCHES 8
uint8_t record_base_pitch[NPITCHES];
uint8_t record_pitch_counter = 0, record_pitch_setting = 0;

void add_record_pitch_change(uint8_t pitch) {
    record_base_pitch[record_pitch_setting++] = pitch;
}


void record_drum(uint8_t drum) {
    record_note_on(drum + RECORD_DRUM_BASE);
}

void record_note_on(uint8_t note) {
    if (event >= RECORD_EVENTS) return;

    if (record_pitch_setting == 0 && note < RECORD_DRUM_BASE)
        record_base_pitch[record_pitch_setting++] = note;

    record_matrix_next[event++] = note;
}

void record_note_off(uint8_t note) {
    if (event >= RECORD_EVENTS) return;

    record_matrix_next[event++] = note + RECORD_OFF_BASE;
}

void reset_recording() {
    record_pitch_counter = 0;
    record_pitch_setting = 0;
    tick = 0; beat = 8;
    uint8_t i, j;
    for (i = 0; i < RECORD_BEATS; ++i) {
        for (j = 0; j < RECORD_EVENTS; ++j) {
            record_matrix[i][j] = NO_VALUE;
        }
    }
}
void update_recording() {
    uint16_t bpm = ADCL | (ADCH << 8);
    ADCSRA |= _BV(ADSC);
    ticks_per_beat = 4000/(bpm+144);
    uint8_t half_beat = ticks_per_beat >> 1;
    uint8_t *beats = record_matrix[beat];
    
    uint8_t i;
    ++tick;
    if (tick >= ticks_per_beat) {
        tick = 0;
        event = 0;
        if (record_mode == RECORD_MODE_RECORDING) {
            for (i = 0; i < RECORD_EVENTS; ++i) {
                beats[i] = record_matrix_next[i];
            }
        }
        ++beat;
        if (beat == RECORD_BEATS) {
            beat = 0;
            if (record_pitch_setting)
                record_pitch_counter = (record_pitch_counter + 1)%record_pitch_setting;
        }
        beats = record_matrix[beat];
        if (record_mode == RECORD_MODE_RECORDING) {
            for (i = 0; i < RECORD_EVENTS; ++i) {
                record_matrix_next[i] = beats[i];
            }
        }
    }
    if (record_mode == RECORD_MODE_RECORDING) {
        if (!(beat&1)) {
            if (tick == half_beat) {
                start_drum(DRUM_HAT);
            }
            else if (beat == 0) {
                if (tick == half_beat - 3)
                    PORTC &= ~_BV(PC4);
                else if (tick == half_beat + 3)
                    PORTC |= _BV(PC4);
            }
            else {
                if (tick == half_beat - 1)
                    PORTC &= ~_BV(PC4);
                else if (tick == half_beat + 1)
                    PORTC |= _BV(PC4);
            }
        }
    }
    if (record_mode != RECORD_MODE_NONE) {
        if (tick == half_beat) {
            for (i = 0; i < RECORD_EVENTS && beats[i] != NO_VALUE; ++i) {
                if (beats[i] < RECORD_OFF_BASE) {
                    start_note(BANK_CPU, RECORDED_KEY(beats[i]));
                }
                else if (beats[i] < RECORD_DRUM_BASE) {
                    stop_note(BANK_CPU, RECORDED_KEY(beats[i]-RECORD_OFF_BASE));
                }
                else if (beats[i] >= RECORD_DRUM_BASE) {
                    start_drum(beats[i]-RECORD_DRUM_BASE);
                }
            }
        }
    }
}
