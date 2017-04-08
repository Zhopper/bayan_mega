#include "MIDI.H"

// Массив нот в формате C=До D=Ре E=Ми F=Фа G=Соль A=Ля B=Си
// Строчная буква d=диез, b=бемоль. Цифра = номер октавы.
// Например Db2 = Ре бемоль 2й октавы
// Индекс в массиве - код ноты по стандарту MIDI
textdatatype notes_names[128][MIDI_NOTE_NAME_LENGTH] PROGMEM = {
"C0 ", "Db0", "D0 ", "Eb0", "E0 ", "F0 ", "Fd0", "G0 ", "Ab0", "A0 ", "Bb0", "B0 ",
"C1 ", "Db1", "D1 ", "Eb1", "E1 ", "F1 ", "Fd1", "G1 ", "Ab1", "A1 ", "Bb1", "B1 ",
"C2 ", "Db2", "D2 ", "Eb2", "E2 ", "F2 ", "Fd2", "G2 ", "Ab2", "A2 ", "Bb2", "B2 ",
"C3 ", "Db3", "D3 ", "Eb3", "E3 ", "F3 ", "Fd3", "G3 ", "Ab3", "A3 ", "Bb3", "B3 ",
"C4 ", "Db4", "D4 ", "Eb4", "E4 ", "F4 ", "Fd4", "G4 ", "Ab4", "A4 ", "Bb4", "B4 ",
"C5 ", "Db5", "D5 ", "Eb5", "E5 ", "F5 ", "Fd5", "G5 ", "Ab5", "A5 ", "Bb5", "B5 ",//C5 = C1 ru / C3 midi
"C6 ", "Db6", "D6 ", "Eb6", "E6 ", "F6 ", "Fd6", "G6 ", "Ab6", "A6 ", "Bb6", "B6 ",
"C7 ", "Db7", "D7 ", "Eb7", "E7 ", "F7 ", "Fd7", "G7 ", "Ab7", "A7 ", "Bb7", "B7 ",
"C8 ", "Db8", "D8 ", "Eb8", "E8 ", "F8 ", "Fd8", "G8 ", "Ab8", "A8 ", "Bb8", "B8 ",
"C9 ", "Db9", "D9 ", "Eb9", "E9 ", "F9 ", "Fd9", "G9 ", "Ab9", "A9 ", "Bb9", "B9 " //0x77
};

textdatatype sounds_names[129][MIDI_NAME_LENGTH] PROGMEM = {
  {"Acoustic Grand Piano \0"}, //0
  {"Bright Piano         \0"}, //1
  {"Electric Grand Piano \0"}, //2
  {"Honkey-Tonk Piano    \0"}, //3
  {"Electric Piano 1     \0"}, //4
  {"Electric Piano 2     \0"}, //5
  {"Harpsichord          \0"}, //6
  {"Clavi                \0"}, //7
  {"Celesta              \0"}, //8
  {"Glockenspiel         \0"}, //9
  {"Music Box            \0"}, //10
  {"Vibraphone           \0"}, //11
  {"Marimba              \0"}, //12
  {"Xylophone            \0"}, //13
  {"Tubular Bells        \0"}, //14
  {"Dulcimer             \0"}, //15
  {"Drawbar Organ        \0"}, //16
  {"Percussive Organ     \0"}, //17
  {"Rock Organ           \0"}, //18
  {"Church Organ         \0"}, //19
  {"Reed Organ           \0"}, //20
  {"Accordion            \0"}, //21
  {"Harmonica            \0"}, //22
  {"Tango Accordion      \0"}, //23
  {"Acoustic Guitar Nylon\0"}, //24
  {"Acoustic Guitar Steel\0"}, //25
  {"Electric Guitar Jazz \0"}, //26
  {"Electric Guitar Clean\0"}, //27
  {"Electric Guitar Muted\0"}, //28
  {"Overdriven Guitar    \0"}, //29
  {"Distortion Guitar    \0"}, //30
  {"Guitar Harmonics     \0"}, //31
  {"Acoustic Bass        \0"}, //32
  {"Electric Bass Finger \0"}, //33
  {"Electric Bass Pick   \0"}, //34
  {"Fretless Bass        \0"}, //35
  {"Slap Bass 1          \0"}, //36
  {"Slap Bass 2          \0"}, //37
  {"Synth Bass 1         \0"}, //38
  {"Synth Bass 2         \0"}, //39
  {"Violin               \0"}, //40
  {"Viola                \0"}, //41
  {"Cello                \0"}, //42
  {"Contrabass           \0"}, //43
  {"Tremolo Strings      \0"}, //44
  {"Pizzicato Strings    \0"}, //45
  {"Orchestral Harp      \0"}, //46
  {"Timpani              \0"}, //47
  {"String Ensemble 1    \0"}, //48
  {"String Ensemble 2    \0"}, //49
  {"Synth Strings 1      \0"}, //50
  {"Synth Strings 2      \0"}, //51
  {"Chor Aahs            \0"}, //52
  {"Voice Oohs           \0"}, //53
  {"Synth Voice          \0"}, //54
  {"Orchestra Hit        \0"}, //55
  {"Trumpet              \0"}, //56
  {"Trombone             \0"}, //57
  {"Tuba                 \0"}, //58
  {"Muted Trumpet        \0"}, //59
  {"French Horn          \0"}, //60
  {"Bass Section         \0"}, //61
  {"Synth Brass 1        \0"}, //62
  {"Synth Brass 2        \0"}, //63
  {"Soprano Sax          \0"}, //64
  {"Alto Sax             \0"}, //65
  {"Tenor Sax            \0"}, //66
  {"Baritone Sax         \0"}, //67
  {"Oboe                 \0"}, //68
  {"English Horn         \0"}, //69
  {"Bassoon              \0"}, //70
  {"Clarinet             \0"}, //71
  {"Piccolo              \0"}, //72
  {"Flyte                \0"}, //73
  {"Recorder             \0"}, //74
  {"Pan Flyte            \0"}, //75
  {"Blown Whistle        \0"}, //76
  {"Shakuhachi           \0"}, //77
  {"Whistle              \0"}, //78
  {"Ocarina              \0"}, //79
  {"Lead 1 Square        \0"}, //80
  {"Lead 2 Sawtooth      \0"}, //81
  {"Lead 3 Calliope      \0"}, //82
  {"Lead 4 Chiff         \0"}, //83
  {"Lead 5 Charang       \0"}, //84
  {"Lead 6 Voice         \0"}, //85
  {"Lead 7 Fifths        \0"}, //86
  {"Lead 8 Bass+Lead     \0"}, //87
  {"Pad 1 New Age        \0"}, //88
  {"Pad 2 Warm           \0"}, //89
  {"Pad 3 Polysynth      \0"}, //90
  {"Pad 4 Choir          \0"}, //91
  {"Pad 5 Bowed          \0"}, //92
  {"Pad 6 Metallic       \0"}, //93
  {"Pad 7 Halo           \0"}, //94
  {"Pad 8 Sweep          \0"}, //95
  {"Fx 1 Rain            \0"}, //96
  {"Fx 2 Sound Track     \0"}, //97
  {"Fx 3 Crystal         \0"}, //98
  {"Fx 4 Atmosphere      \0"}, //99
  {"Fx 5 Brightness      \0"}, //100
  {"Fx 6 Goblins         \0"}, //101
  {"Fx 7 Echoes          \0"}, //102
  {"Fx 8 Sci-Fi          \0"}, //103
  {"Sitar                \0"}, //104
  {"Banjo                \0"}, //105
  {"Shamisen             \0"}, //106
  {"Koto                 \0"}, //107
  {"Kalimba              \0"}, //108
  {"Bag Pipe             \0"}, //109
  {"Fiddle               \0"}, //110
  {"Shanai               \0"}, //111
  {"Tinkle Bell          \0"}, //112
  {"Agogo                \0"}, //113
  {"Steel Drums          \0"}, //114
  {"Woodblock            \0"}, //115
  {"Taiko Drum           \0"}, //116
  {"Melodic Tom          \0"}, //117
  {"Synth Drum           \0"}, //118
  {"Reverse Cymbal       \0"}, //119
  {"Guitar Fret Noise    \0"}, //120
  {"Breath Noise         \0"}, //121
  {"Seashore             \0"}, //122
  {"Bird Tweet           \0"}, //123
  {"Telephone Ring       \0"}, //124
  {"Helicopter           \0"}, //125
  {"Applause             \0"}, //126
  {"Gunshot              \0"}, //127
  {"                     \0"}, //128
};

