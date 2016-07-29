/* record.h -
     Sequence recording and playback routines

   (c) 2016 Ken Rossato
 */

#ifndef RECORD_H
#define RECORD_H

#define RECORD_MODE_NONE      0
#define RECORD_MODE_RECORDING 1
#define RECORD_MODE_PLAYBACK  2

extern uint8_t record_mode;

void reset_recording();
void record_drum(uint8_t drum);
void record_note_on(uint8_t note);
void record_note_off(uint8_t note);
void update_recording();
void add_record_pitch_change(uint8_t pitch);

#endif
