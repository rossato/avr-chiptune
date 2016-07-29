/* drum.h -
     Drum definitions and playback queue

   (c) 2016 Ken Rossato
 */

#ifndef DRUM_H
#define DRUM_H

#include "main.h"

#define DRUM_HAT 0
#define DRUM_TOM_HI 1
#define DRUM_TOM 2
#define DRUM_TOM_LO 3
#define DRUM_CYMBOL 4
#define DRUM_SNARE_NOISE 5
#define DRUM_SNARE_TONE 6
#define DRUM_SNARE_LO_TONE 7
#define DRUM_KICK 8
#define DRUM_KICK_SQUARE 9
#define NDRUMS 10

struct drum_t {
    uint16_t freq;
    int16_t slide;
    uint8_t shape;
    const struct instrument_t *instrument;
    uint8_t duration;
};
extern const struct drum_t drums[];
extern const struct drum_t *drum_machine[];

uint8_t allocate_drum_note(uint8_t drum);

#endif
