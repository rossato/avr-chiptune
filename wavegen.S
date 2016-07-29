        ;; wavegen.S - Audio synthesis routine for 8-bit synthesizer project
        ;; (c) 2016 Ken Rossato

#include <avr/io.h>
#include "constants.h"

        .global TIMER1_COMPA_vect
        .extern channels
        ;; .extern pcm_sample   ; PCM output is todo

        ;; noiseshift_lo, noiseshift_hi assigned to r2, r3

TIMER1_COMPA_vect:
        push r18                        ; master out
        push r19                        ; loop var i
        in r19, _SFR_IO_ADDR(SREG)      ;
        push r19                        ;
        push r21                        ; phase lo / shape / duty
        push r22                        ; phase hi / channel out
        push r26                        ; freq lo and sample lo ("X")
        push r27                        ; freq hi and sample hi
        push r30                        ; channels lo ("Z")
        push r31                        ; channels hi

        ldi r18, 0x00
        ldi r19, NCHANNELS
        ldi r30, lo8(channels)
        ldi r31, hi8(channels)

spicheck1:
        ;; in r21, _SFR_IO_ADDR(SPSR)
        ;; sbrs r21, SPIF
        ;; rjmp spicheck1
        sbi _SFR_IO_ADDR(PORTB), PB2

channel_loop:
        ld r26, Z                       ; freq lo
        ldd r27, Z+1                    ; freq hi
        ldd r21, Z+2                    ; phase lo
        ldd r22, Z+3                    ; phase hi
        add r21, r26                    ; Add freq to phase
        adc r22, r27                    ;
        std Z+2, r21                    ; Store phase back
        std Z+3, r22                    ;
        ldd r21, Z+4                    ; shape
        sbrc r21, SHAPE_SQUARE_BIT      ;
        rjmp square                     ;
        sbrc r21, SHAPE_TRIANGLE_BIT    ;
        rjmp triangle                   ;
        sbrs r21, SHAPE_NOISE_BIT       ;
        ;; rjmp noise
        ;; sbrs r21, SHAPE_PCM_BIT
        rjmp nextloop

        ;; pcm:
        ;; lds r26, pcm_sample
        ;; ldi r27, hi8(pcm_data)
        ;; ld r22, X
        ;; lds r22, pcm_sample
        ;; rjmp addsound

noise:
        ;; This is a 15-bit LFSR using the polynomial x^15 + x^14 + 1.
        ;; See https://en.wikipedia.org/wiki/Linear-feedback_shift_register
        brcc render_noise               ; carry flag is from "adc r22, r27"
        lsl r2                          ; noiseshift_lo
        rol r3                          ; noiseshift_hi
        brvc render_noise
        ldi r22, 2                      ; The shift register is left-justified:
        or r2, r22                      ; New bit2 = !(bit16 xor bit15)

render_noise:
        ldd r22, Z+6                    ; volume
        sbrs r2, 1
        neg r22
        rjmp addsound

triangle:
        lsr r22
        sbrs r22, 6
        rjmp triangle_up

triangle_down:
        ;; 0x40 to 0x7f to yield 0x20 to -0x1F
        subi r22, 0x60
        neg r22
        rjmp addsound

triangle_up:
        ;; 0x00 to 0x3F to yield -0x20 to 0x1F
        subi r22, 0x20
        rjmp addsound

square:
        ldd r21, Z+5                    ; duty
        cp r21, r22                     ;
        ldd r22, Z+6                    ; volume
        brsh addsound                   ;
        neg r22                         ;

addsound:
        ;; Clamp testing didn't help that much, it isn't worth the cycles
        add r18, r22

nextloop:
        adiw r30, 11                    ; sizeof(struct channel_t)
        dec r19
        brne channel_loop

home_stretch:
        ;; The DAC takes 12 bits right justified, MSB first.
        ;; DAC options pad the first 4 bits.
        cbi _SFR_IO_ADDR(PORTB), PB2
        ldi r19, 0x80
        add r19, r18
        swap r19
        andi r19, 0x0F
        ori r19, DAC_DEFAULT
        out _SFR_IO_ADDR(SPDR), r19

        swap r18
        andi r18, 0xF0

        pop r31
        pop r30
        pop r27
        pop r26
        pop r22
        pop r21
        pop r19
        out _SFR_IO_ADDR(SREG), r19
        pop r19

spicheck2:
        ;; in r19, _SFR_IO_ADDR(SPSR)
        ;; sbrs r19, SPIF
        ;; rjmp spicheck2
        out _SFR_IO_ADDR(SPDR), r18

        pop r18
        reti
