/* drum.c -
     Drum definitions and playback queue

   (c) 2016 Ken Rossato
 */

#include "main.h"
#include "drum.h"

uint8_t drum_machine_page = 0;
const struct drum_t *drum_machine[2] = {&drums[0], &drums[0]};

const struct drum_t drums[NDRUMS] = {
    [DRUM_HAT] = {
        .freq = 0xFFFF,
        .slide = 0,
        .shape = SHAPE_NOISE,
        .instrument = &instruments[INSTRUMENT_HAT],
        .duration = 5
    },
    [DRUM_TOM_HI] = {
        .freq = 0x030A, // C4
        .slide = -0x30,
        .shape = SHAPE_TRIANGLE,
        .instrument = &instruments[INSTRUMENT_DEFAULT],
        .duration = 8,
    },
    [DRUM_TOM] = {
        .freq = 0x028E, // A3
        .slide = -0x30,
        .shape = SHAPE_TRIANGLE,
        .instrument = &instruments[INSTRUMENT_DEFAULT],
        .duration = 8,
    },
    [DRUM_TOM_LO] = {
        .freq = 0x0247, // G3
        .slide = -0x30,
        .shape = SHAPE_TRIANGLE,
        .instrument = &instruments[INSTRUMENT_DEFAULT],
        .duration = 8,
    },
    [DRUM_CYMBOL] = {
        .freq = 0x8000,
        .slide = 0,
        .shape = SHAPE_NOISE,
        .instrument = &instruments[INSTRUMENT_CYMBOL],
        .duration = 15
    },
    [DRUM_SNARE_NOISE] = {
        .freq = 0x1800,
        .slide = 0x1733,
        .shape = SHAPE_NOISE,
        .instrument = &instruments[INSTRUMENT_SNARE],
        .duration = 15
    },
    [DRUM_SNARE_TONE] = {
        .freq = 0x030A, // C4
        .slide = -0x20,
        .shape = SHAPE_TRIANGLE,
        .instrument = &instruments[INSTRUMENT_DEFAULT],
        .duration = 15
    },
    [DRUM_SNARE_LO_TONE] = {
        .freq = 0x0269, // G#3
        .slide = -0x14,
        .shape = SHAPE_TRIANGLE,
        .instrument = &instruments[INSTRUMENT_DEFAULT],
        .duration = 15
    },
    [DRUM_KICK] = {
        .freq = 0x15B, // A#2
        .slide = -0x10,
        .shape = SHAPE_TRIANGLE,
        .instrument = &instruments[INSTRUMENT_DEFAULT],
        .duration = 15
    },
    [DRUM_KICK_SQUARE] = {
        .freq = 0x15B,
        .slide = -0x10,
        .shape = SHAPE_SQUARE,
        .instrument = &instruments[INSTRUMENT_CYMBOL],
        .duration = 15
    }
};

uint8_t allocate_drum_note(uint8_t drum) {
    drum_machine_page = (drum_machine_page + 1) & 2;
    drum_machine[drum_machine_page] = &drums[drum];
    return DRUM_MACHINE_1 + drum_machine_page;
}
