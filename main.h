/* main.h -
     Setup, audio maintenance, and bank/channel/note management routines

   (c) 2016 Ken Rossato
 */

#ifndef MAIN_H
#define MAIN_H

#include "constants.h"

#define NULL ( (void *) 0)

#define INSTRUMENT_DURATION 15

struct instrument_t {
    uint8_t volume[INSTRUMENT_DURATION];
};
extern const struct instrument_t instruments[];

struct bank_t {
    uint8_t shape;
    uint8_t duty;
    uint8_t mode;
    const struct instrument_t *instrument;
    uint8_t first_note;
    uint8_t end_note;
};
extern struct bank_t banks[];

struct channel_t {
    volatile uint16_t freq;
    volatile uint16_t phase;
    volatile uint8_t shape;
    volatile uint8_t duty;
    volatile int8_t volume;
    struct bank_t *bank;
    uint8_t note;
    uint8_t frame;
};
extern struct channel_t channels[];

void set_cpu_bank_mode(uint8_t bank);
void notify_bank_mode_changed(uint8_t bank);

void start_note(uint8_t note);
void stop_note(uint8_t note);

#endif
