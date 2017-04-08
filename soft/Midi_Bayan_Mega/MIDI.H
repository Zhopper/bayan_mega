#ifndef MIDI_H
#define MIDI_H

#include <avr/pgmspace.h>
#define textdatatype const uint8_t

#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_POLY_KEY 0xA0
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CHANNEL_PRESSURE 0xD0
#define MIDI_PITCH_WHEEL 0xE0

#define MIDI_CONTROL_MODULATION_WHEEL 1
#define MIDI_CONTROL_CHANNEL_VOLUME 7
#define MIDI_CONTROL_EXPRESSION 11
#define MIDI_CONTROL_REVERB 91
#define MIDI_CONTROL_CHORUS 93


#define MIDI_MAX_VALUE 127
#define MIDI_NAME_LENGTH 22
#define MIDI_NOTE_NAME_LENGTH 3

extern const char notes[0x7F][3];
extern textdatatype notes_names [128][MIDI_NOTE_NAME_LENGTH];
extern textdatatype sounds_names[129][MIDI_NAME_LENGTH];

#endif

