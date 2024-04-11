typedef struct MidiHeader
{
    unsigned short formatType;
    unsigned short numberOfTracks;
    unsigned short timeDivision;
} MidiHeader;

typedef struct MidiEvent
{
    unsigned int deltaTime;
    unsigned int absoluteTime;
    unsigned char eventType;       // 0XFF(meta event) || 0xF0(sysex) || 0xF7(sysex)
    unsigned char metaType;        // for metaevents, 	0x59 for key signature, 0x04 for instrument
    unsigned char keySignature[2]; // 2 bytes worth of data
    unsigned char timeSignature[4];
    char *instrument;
    unsigned char param1;
    unsigned char param2;
    struct MidiEvent *next;
} MidiEvent;

typedef struct MidiTrack
{
    MidiEvent *events;
} MidiTrack;

typedef struct MidiFile
{
    MidiHeader header;
    MidiTrack *tracks;
} MidiFile;

// keys for the key signature e.g. C Major, A Minor
// the int has values between -7 and 7 and specifies the key signature
// in terms of number of flats (if negative) or sharps (if positive).
typedef struct key
{
    enum Scale
    {
        MAJOR,
        MINOR
    } scale;
    int fifths;
} Key;

typedef enum notes
{
    C_NOTE,
    D_NOTE,
    E_NOTE,
    F_NOTE,
    G_NOTE,
    A_NOTE,
    B_NOTE,
} NotesEnum;

// https://ultimatemusictheory.com/piano-key-numbers/
// 8 octaves for a piano, midi supports 10 octaves, this pitches is for piano for now
// aka scientific pitch notation
typedef enum pitches
{
    A0,
    B0,
    C1,
    D1,
    E1,
    F1,
    G1,
    A1,
    B1,
    C2,
    D2,
    E2,
    F2,
    G2,
    A2,
    B2,
    C3,
    D3,
    E3,
    F3,
    G3,
    A3,
    B3,
    C4,
    D4,
    E4,
    F4,
    G4,
    A4,
    B4,
    C5,
    D5,
    E5,
    F5,
    G5,
    A5,
    B5,
    C6,
    D6,
    E6,
    F6,
    G6,
    A6,
    B6,
    C7,
    D7,
    E7,
    F7,
    G7,
    A7,
    B7,
    C8,
} PitchesEnum;

// used with pitch for sharp, flat and natural
typedef enum accidentals
{
    FLAT,
    SHARP,
    NATURAL
} AccidentalsEnum;

typedef struct Note
{
    PitchesEnum pitch;
    AccidentalsEnum accidental;
} Note;

typedef struct Measure
{
    Note notes[10];
    Key key;
} Measure;

typedef struct PreMusicXML
{
    char key[10];
    MidiTrack tracks[];
} PreMusicXML;

typedef struct MidiNote
{
    char *noteType;
    int isChord;
    int duration;
    int midiNoteNumber;
    int keySignature;
    int beats;
    int beatsType;
    char *instrument;
    struct MidiNote *next; // move to next event with midiEvent.next
} MidiNote;

unsigned int readVariableLengthQuantity(FILE *file, unsigned int *bytesRead); // For Delta time, since its variable Length

MidiEvent *createMidiEvent(unsigned int deltaTime, unsigned int absoluteTime, unsigned char eventType, unsigned char metaType, unsigned char *keySignature, unsigned char *timeSignature, char *instrument,
                           unsigned char param1, unsigned char param2, struct MidiEvent *next);

MidiNote *createMidiNoteForTrack(MidiFile *midiFile, MidiTrack track, int maxNotes);

MidiFile *readMidiFile(const char *filename);

void createHeaders(FILE *file);

void createPartList(FILE *file, int numberOfParts);

void createPart(FILE *file, int *currentPartNumber, int division, int fifths, int beats, int beatsType, char *instrumentType);

void createNote(FILE *file, MidiNote midiNote);

void closePart(FILE *file);

void closeHeaders(FILE *file);

MidiEvent *readMidiTrackEvents(FILE *file, unsigned int trackLength);
MidiEvent *processMidiEvent(unsigned char eventByte, unsigned int *bytesRead,
                            FILE *file, unsigned int deltaTime, unsigned int currentAbsoluteTime);
MidiEvent *processMetaEvent(unsigned char metaType, unsigned int *bytesRead,
                            unsigned int metaLength, FILE *file, unsigned int deltaTime,
                            unsigned int currentAbsoluteTime);
