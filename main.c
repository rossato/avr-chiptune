/* main.c -
     Setup, audio maintenance, and bank/channel/note management routines

   (c) 2016 Ken Rossato
 */

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/setbaud.h>
#include "main.h"
#include "drum.h"
#include "interface.h"
#include "keys.h"
#include "midi.h"
#include "record.h"

register uint8_t noiseshift_lo asm("r2");
register uint8_t noiseshift_hi asm("r3");

// volatile uint8_t pcm_sample = 0; /* TODO: Autoupdate by PCM interrupt */

/* Calculated from 12-tone equal temperament, A4 = 440Hz,
   see https://en.wikipedia.org/wiki/Piano_key_frequencies.
   freq = 65536 / ((F_OSC / OCR2A) / F_note)  
                 |--Num samples in a note---|
 */
const uint16_t freqs[TOTAL_NOTES] = {
/* 41667Hz, OCR2A = 47, 8x prescale */
    /* 0x2B, 0x2E, 0x31, 0x33, 0x36, 0x3A, 0x3D, 0x41,  */
    /* 0x45, 0x49, 0x4D, 0x52, 0x57, 0x5C, 0x61, 0x67,  */
    /* 0x6D, 0x73, 0x7A, 0x82, 0x89, 0x91, 0x9A, 0xA3,  */
    /* 0xAD, 0xB7, 0xC2, 0xCE, 0xDA, 0xE7, 0xF5, 0x103,  */
    /* 0x113, 0x123, 0x134, 0x147, 0x15A, 0x16F, 0x184, 0x19C,  */
    /* 0x1B4, 0x1CE, 0x1E9, 0x206, 0x225, 0x246, 0x269, 0x28D,  */
    /* 0x2B4, 0x2DD, 0x309, 0x337, 0x368, 0x39C, 0x3D3, 0x40D,  */
    /* 0x44B, 0x48C, 0x4D1, 0x51A, 0x568, 0x5BA, 0x612, 0x66E,  */
    /* 0x6D0, 0x738, 0x7A5, 0x81A, 0x895, 0x918, 0x9A2, 0xA35,  */
    /* 0xAD0, 0xB75, 0xC23, 0xCDC, 0xDA0, 0xE6F, 0xF4B, 0x1034,  */
    /* 0x112A, 0x1230, 0x1344, 0x146A, 0x15A0, 0x16EA, 0x1846, 0x19B8 */
/* 44077Hz, OCR1AH = 1, OCR1AL = 106 (sum=362) */
    0x29, 0x2B, 0x2E, 0x31, 0x34, 0x37, 0x3A, 0x3D,
    0x41, 0x45, 0x49, 0x4D, 0x52, 0x57, 0x5C, 0x61,
    0x67, 0x6D, 0x74, 0x7B, 0x82, 0x8A, 0x92, 0x9A,
    0xA4, 0xAD, 0xB8, 0xC2, 0xCE, 0xDA, 0xE7, 0xF5,
    0x104, 0x113, 0x123, 0x135, 0x147, 0x15B, 0x16F, 0x185,
    0x19C, 0x1B5, 0x1CF, 0x1EA, 0x207, 0x226, 0x247, 0x269,
    0x28E, 0x2B5, 0x2DE, 0x30A, 0x338, 0x369, 0x39D, 0x3D4,
    0x40E, 0x44C, 0x48E, 0x4D3, 0x51C, 0x56A, 0x5BD, 0x614,
    0x671, 0x6D3, 0x73A, 0x7A8, 0x81D, 0x899, 0x91B, 0x9A6,
    0xA39, 0xAD4, 0xB79, 0xC28, 0xCE1, 0xDA5, 0xE75, 0xF51,
    0x103A, 0x1131, 0x1237, 0x134C, 0x1472, 0x15A9, 0x16F3, 0x1850,
};

//Test tone.
/* ISR(TIMER1_COMPA_vect) { */
/*     PORTB &= ~(_BV(PB2)); */
/*     channels[0].phase += 0x004D; */
/*     if (channels[0].phase > 0x8000) { */
/*         SPDR = DAC_DEFAULT | 0x09; */
/*     } */
/*     else { */
/*         SPDR = DAC_DEFAULT | 0x07; */
/*     } */
/*     while (!(SPSR & _BV(SPIF))); */
/*     SPDR = 0x00; */
/*     while (!(SPSR & _BV(SPIF))); */
/*     PORTB |= _BV(PB2); */
/* } */

const struct instrument_t instruments[NINSTRUMENTS] = {
    [INSTRUMENT_DEFAULT] = {
        .volume = {0x0C, 0x18, 0x14, 0x10, 0x0E,
                   [5 ... 14] = 0x0C}
    },
    [INSTRUMENT_HAT] = {
        .volume = {0x18, 0x14, 0x08, 0x08, 0x08,
                   0x08, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00}
    },
    [INSTRUMENT_CYMBOL] = {
        .volume = {0x1A, 0x1A, 0x1A, 0x12, 0x12,
                   0x12, 0x12, 0x12, 0x0A, 0x0A,
                   0x0A, 0x0A, 0x0A, 0x02, 0x02}
    },
    [INSTRUMENT_SNARE] = {
        .volume = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
                   0x10, 0x10, 0x10, 0x10, 0x10,
                   0x10, 0x00, 0x00, 0x00, 0x00}
    },
    // This one is a fun breathy tone.  Maybe later.
    /* { */
    /*     .volume = {0x08, 0x10, 0x18, 0x16, 0x12, */
    /*                0x10, 0x0E, 0x0C, 0x0C, 0x0C} */
    /* }, */
};

struct bank_t banks[NBANKS] = {
    [BANK_KEYS] = {
        .shape = SHAPE_SQUARE,
        .duty = 0x80,
        .mode = MODE_VIBRATO,
        .instrument = INSTRUMENT_DEFAULT,
        .notes[0 ... (TOTAL_NOTES-1)] = {
            .channel = NO_VALUE
        }
    },
    [BANK_MIDI] = {
        .shape = SHAPE_TRIANGLE,
        .duty = 0x80,
        .mode = MODE_VIBRATO,
        .instrument = INSTRUMENT_DEFAULT,
        .notes[0 ... (TOTAL_NOTES-1)] = {
            .channel = NO_VALUE
        }
    },
    [BANK_CPU] = {
        .shape = SHAPE_SQUARE,
        .duty = 0x80,
        .mode = MODE_NONE,
        .instrument = INSTRUMENT_DEFAULT,
        .notes[0 ... (TOTAL_NOTES-1)] = {
            .channel = NO_VALUE
        }
    }
};

struct channel_t channels[NCHANNELS] = {
    [0 ... (NCHANNELS-1)] = {
        .shape = SHAPE_NONE,
        .duty = 0x80,
        .volume = 0x00,
        .bank = NO_VALUE,
        .note = NO_VALUE,
        .frame = 0,
    }
};

void set_cpu_bank_mode(uint8_t source_bank) {
    if (banks[source_bank].shape == SHAPE_DRUM)
        source_bank = (source_bank+1)%2;

    banks[BANK_CPU].shape = banks[source_bank].shape;
    banks[BANK_CPU].duty = banks[source_bank].duty;
    banks[BANK_CPU].mode = banks[source_bank].mode;
}

void notify_bank_mode_changed(uint8_t bank) {
    if (record_mode == RECORD_MODE_RECORDING)
        set_cpu_bank_mode(bank);
}

/* inline void channel_off(struct channel_t *channel, struct bank_t *bank) { */
/*     channel->shape = SHAPE_NONE; */
/*     channel->bank = NO_VALUE; */
/*     if (channel->note != NO_VALUE) { */
/*         bank->notes[channel->note].channel = NO_VALUE; */
/*         channel->note = NO_VALUE; */
/*     } */
/* } */

/* Time periods are in maintenance ticks, or 100ths of a second */
#define ARP_DURATION 4

#define VIBRATO_START 40
#define VIBRATO_DURATION 24

#define ECHO_START 100
#define ECHO_DURATION 50

// Amount to right-bitshift for the freq change
// Zero is no change, otherwise higher is less change.
// A semitone is about 1.0594 above the previous note,
//  so 5 is less than a semitone, 4 is more.
const int8_t vibrato_amount[VIBRATO_DURATION] = { 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0,
                                                 -6, -6, -6, -6, -6, -6, 0, 0, 0, 0, 0, 0 };

/* Called at roughly 100Hz */
void update_audio() {
    uint8_t channel;
    struct channel_t *p_channel = channels;
    for (channel = 0; channel < NCHANNELS; ++channel, ++p_channel) {

        uint8_t bank = p_channel->bank;
        if (bank == NO_VALUE) continue;
        else if (bank == BANK_DRUMS) {
            update_drum(channel);
            continue;
        }

        struct bank_t *p_bank = &banks[bank];

        uint8_t frame = p_channel->frame;
        uint8_t note = p_channel->note;
        struct note_t *p_note = &p_bank->notes[note];

        // Start and stop notes
        if (note == NO_VALUE || p_note->channel != channel) {
            p_note = p_bank->notes;
            for (note = 0; note < TOTAL_NOTES; ++note, ++p_note) {
                if (p_note->channel == channel) {
                    p_channel->note = note;
                    p_channel->freq = freqs[note];
                    p_channel->bank = bank;
                    p_channel->frame = frame = 0;
                    p_channel->volume = 0;
                    p_channel->duty = p_bank->duty;
                    p_channel->shape = p_bank->shape;
                    break;
                }
            }
            if (note == TOTAL_NOTES) {
                p_channel->bank = NO_VALUE;
                p_channel->shape = SHAPE_NONE;
                p_channel->note = NO_VALUE;
                continue;
            }
        }
        // Arpeggiate
        else if (frame == ARP_DURATION) {
            uint8_t old_note = note;
            for (--note, --p_note; note != old_note; --note, --p_note) {
                if (note >= TOTAL_NOTES) {
                    note += TOTAL_NOTES;
                    p_note += TOTAL_NOTES;
                }
                if (p_note->channel == channel) {
                    p_channel->note = note;
                    p_channel->freq = freqs[note];
                    p_channel->frame = 0;
                    break;
                }
            }
        }

        // Attack-decay-sustain
        const uint8_t *instrument_volume = instruments[p_bank->instrument].volume;
        if (frame < INSTRUMENT_DURATION) {
            p_channel->volume = instrument_volume[frame];
        }

        // Echo
        if (frame >= ECHO_START) {
            if (frame < ECHO_START + INSTRUMENT_DURATION) {
                p_channel->volume = instrument_volume[frame-ECHO_START] >> 1;
            }
            else if (frame >= ECHO_START + ECHO_DURATION) {
                p_channel->shape = SHAPE_NONE;
                p_channel->bank = NO_VALUE;
                p_channel->note = NO_VALUE;
                p_note->channel = NO_VALUE;
                continue;
            }
        }
        // Vibrato
        else {
            if (frame >= VIBRATO_START + VIBRATO_DURATION)
                p_channel->frame = (frame -= VIBRATO_DURATION);

            if ((p_bank->mode & MODE_VIBRATO) && frame >= VIBRATO_START) {
                int8_t amount = vibrato_amount[frame - VIBRATO_START];
                uint16_t freq = freqs[note];
                if (amount > 0)
                    p_channel->freq = freq + (freq >> amount);
                else if (amount < 0)
                    p_channel->freq = freq - (freq >> -amount);
                else
                    p_channel->freq = freq;
            }
        }

        ++(p_channel->frame);
    }
}

uint8_t next_channel = 0;
uint8_t allocate_channel(uint8_t bank, uint8_t note) {
// Channel algorithm, by priority:
// (1) If the bank is arpeggiating or we're at max triangles, find the bank channel
// (2) Find an empty channel
// (3) Find a same-bank channel and arpeggiate on it
// (4) Find any channel not occupied by burst and take it over

    uint8_t n_triangles = 0;
    uint8_t bank_channel = NO_VALUE, empty_channel = NO_VALUE,
        last_chance = next_channel, channel;

    {
        struct channel_t *p_channel = channels;
        for (channel = 0; channel < NCHANNELS; ++p_channel, ++channel) {
            uint8_t channel_bank = p_channel->bank;
            if (channel_bank == NO_VALUE) {
                empty_channel = channel;
            }
            else if (p_channel->shape == SHAPE_TRIANGLE) {
                ++n_triangles;
            }
            if (channel_bank == bank) {
                bank_channel = channel;
            }
            if (channel_bank < BANK_CPU) {
                last_chance = channel;
            }
        }
    }
    ++next_channel; if (next_channel > NCHANNELS) next_channel = 0;

    struct bank_t *p_bank = &banks[bank];

    if ((bank_channel != NO_VALUE) &&
        ((p_bank->mode & MODE_ARPEGGIO) ||
         (p_bank->shape == SHAPE_TRIANGLE && n_triangles >= 4)))  {
        return bank_channel;
    }
    else if (empty_channel != NO_VALUE) {
        channels[empty_channel].bank = bank;
        return empty_channel;
    }
    else if (bank_channel != NO_VALUE) {
        return bank_channel;
    }

    uint8_t last_bank = channels[last_chance].bank;
    channels[last_chance].bank = bank;
    struct note_t *p_note = banks[last_bank].notes;

    for (note = 0; note < TOTAL_NOTES; ++note, ++p_note) {
        if (p_note->channel == last_chance) {
            p_note->channel = NO_VALUE;
            start_note(last_bank, note); // Go arpeggiate with your own bank
        }
    }
    return last_chance;
}

void start_note(uint8_t bank, uint8_t note) {
    struct bank_t *p_bank = &banks[bank];
    struct note_t *p_note = &p_bank->notes[note];            
    struct channel_t *p_channel = &channels[p_note->channel];

    if (p_note->channel != NO_VALUE) {
        if (p_channel->frame >= ECHO_START)
            p_channel->frame = 0;
        return;
    }

    if (record_mode == RECORD_MODE_RECORDING && bank < BANK_CPU) {
        if (p_bank->shape == SHAPE_DRUM)
            record_drum(note % NDRUMS);
        else
            record_note_on(note);
    }

    if (p_bank->shape == SHAPE_DRUM) {
        p_note->channel = start_drum(note % NDRUMS);
        return;
    }

    uint8_t channel = allocate_channel(bank, note);
    p_note->channel = channel;

    channels[channel].frame = 0;

    if (p_bank->mode & MODE_BURST) {
        start_drum(DRUM_HAT);
    }
}

void stop_note(uint8_t bank, uint8_t note) {
    struct bank_t *p_bank = &banks[bank];
    struct note_t *p_note = &p_bank->notes[note];
    if (p_note->channel == NO_VALUE) return;

    if (record_mode == RECORD_MODE_RECORDING && bank < BANK_CPU) {
        record_note_off(note);
    }

    struct channel_t *p_channel = &channels[p_note->channel];
    
    if (p_bank->mode & MODE_ECHO) {
        if (p_channel->frame < ECHO_START) {
            p_channel->frame = ECHO_START;
            p_channel->freq = freqs[note];
        }
    }
    else {
        p_note->channel = NO_VALUE;
    }
}

void setup_cpu() {
/* SPI interface to DAC    Parallel shift out
   PB5: SCK,  PB4: MISO    PB0: Clock
   PB3: MOSI, PB2: SS      PB1: Out           */
    DDRB  |= _BV(PB5) | _BV(PB3) | _BV(PB2) | _BV(PB1) | _BV(PB0);
    PORTB |= _BV(PB2);
    SPCR  |= _BV(SPE) | _BV(MSTR);
    SPSR  |= _BV(SPI2X);

/* Pull-up the keyboard */ 
    PORTD |= 0xFE;

/* Pull-up the buttons and turn the LEDs off
   PC5: Right light   PC3: Left button    PC1: Top button
   PC4: Left light    PC2: Right button                   */
    DDRC = _BV(PC5) | _BV(PC4);
    PORTC = _BV(PC5) | _BV(PC4) | _BV(PC3) | _BV(PC2) | _BV(PC1);

/* Sampling clock */
//    TCCR2A |= _BV(WGM21);  /* CTC mode */
//    TCCR2B |= _BV(CS21);   /* 8x prescale */
//    OCR2A  = 47;           /* 41.667kHz, gives 384 clocks     */
//    TIMSK2 |= _BV(OCIE2A); /*  with good high note pitch      */

    TCCR1B |= _BV(WGM12) | _BV(CS10); /* CTC, 1x prescale */
    OCR1AH = 1; OCR1AL = 106;         /* 44.077kHz */
    TIMSK1 |= _BV(OCIE1A);

/* Maintenance loop */
    TCCR0A |= _BV(WGM01);            /* CTC */
    TCCR0B |= _BV(CS02) | _BV(CS00); /* 1024x prescale */
    OCR0A = 155;                     /* 100.16 Hz */
    // No need for an interrupt, we'll check it manually
    
/* Set up USART for MIDI */
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
    UCSR0B |= _BV(RXEN0) | _BV(RXCIE0);
    UCSR0C |= _BV(UCSZ01) | _BV(UCSZ00); /* 8n1 */

/* ADC */
    PRR &= ~_BV(PRADC);
    ADCSRA |= _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    DIDR0 |= _BV(ADC0D);
}

int main(void) {
    //wdt_disable();
    wdt_enable(WDTO_500MS);
    setup_cpu();

    noiseshift_lo = 2;
    
    SPDR = 0x00; // dummy to force SPIF high
    sei();

    uint8_t prescale = 0;
    while (1) {
        if (TIFR0 & _BV(OCF0A)) {
            TIFR0 |= _BV(OCF0A);

            wdt_reset();

            if (prescale) {
                scan_interface();
                update_interface();
                update_recording();
            }
            prescale = !prescale;
            scan_keys();
            update_audio();
        }
        if (midi_next_message != midi_last_message)
            scan_midi();
    }

    return 0;
}
