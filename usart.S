        ;; usart.S - MIDI receive interrupt
        ;; (c) 2016 Ken Rossato

#include <avr/io.h>

        .global USART_RX_vect
        .extern midi_ring_buffer
        .extern midi_last_message

USART_RX_vect:
        push r0
        in r0, _SFR_IO_ADDR(SREG)
        push r0
        push r30
        push r31

        lds r30, midi_last_message
        ldi r31, hi8(midi_ring_buffer)
        lds r0, _SFR_MEM_ADDR(UDR0)
        st Z+, r0
        andi r30, 63
        ori r30, lo8(midi_ring_buffer)
        sts midi_last_message, r30

        pop r31
        pop r30
        pop r0
        out _SFR_IO_ADDR(SREG), r0
        pop r0  
        reti                       
