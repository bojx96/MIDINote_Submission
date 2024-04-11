#include <stdio.h>
#include <stdlib.h>
#include "main.h"

unsigned char tempoData[3];
unsigned char timeSigData[4];
unsigned char runningStatus;
unsigned char keySigData[2];
unsigned char instrumentNameData[100];

typedef enum
{
    ParseFailureState,
    InitialState,
    MetaEventState = 0xFF,
    SysexEventState1 = 0xF0,
    SysexEventState2 = 0xF7,
    MidiEventState = 0x80,
    EndTrackState = 0x2F,
    EndOfFileState

} ParserState;

typedef struct
{
    ParserState currentState;
    unsigned char byte;
} ParserFSM;

void initParserFSM(ParserFSM *fsm, unsigned char status)
{
    fsm->currentState = InitialState;
    fsm->byte = 0;
}

int readMidiHeader(FILE *file, struct MidiHeader *header)
{
    /* Header Contains
    unsigned short formatType;
    unsigned short numberOfTracks;
    unsigned short timeDivision;
    */
    unsigned char buffer[100];     /* u char is 1 byte*/
    unsigned char msb_mask = 0x80; /* mask for b10000000*/
    size_t bytesRead,
        i;

    /* Step 1: Read and make sure its MThd*/
    unsigned char mthd[] = {0x4d, 0x54, 0x68, 0x64};
    /* hex values for MThd, 1 bytes per 2 hex char */
    /* Step 2: If is Mthd proceed, else return error*/
    bytesRead = fread(buffer, 14, 1, file);
    for (i = 0; i < 4; ++i)
    {
        if (buffer[i] != mthd[i])
        {

            return 0;
        }
    }
    printf("MIDI Header OK\n");
    /* Step 3: Assign all variables, formatType, numberOfTracks, timeDivision*/
    /* Start from 9th byte, header_length is 5th to 8th bit, has no useful data as it is fixed */
    header->formatType = (((unsigned short)buffer[8]) << 8) | (((unsigned short)buffer[9]) << 0);
    header->numberOfTracks = (((unsigned short)buffer[10]) << 8) | (((unsigned short)buffer[11]) << 0);
    /* Division is the delta time unit and has two types of formats */
    /* if the MSB is 1, checked using bitwise AND then it uses negative SMPTE format */
    /* else it uses ticks per quarter note */
    /* for now, copy over for both using big endian */
    if (buffer[12] & msb_mask)
    {
        header->timeDivision = (((unsigned short)buffer[12]) << 8) | (((unsigned short)buffer[13]) << 0);
    }
    else
    {
        header->timeDivision = (((unsigned short)buffer[12]) << 8) | (((unsigned short)buffer[13]) << 0);
    }

    printf("%04x ", header->formatType); /* %04x means print at least 4 digits, pad with 0 for blank. 2 bytes in hex -> 4 digits*/
    printf("%04x ", header->numberOfTracks);
    printf("%04x ", header->timeDivision);
    printf("Midi HEADER Read OK.\n");
    printf("END\n");
    /* Step 3.1: Need to find out how many bytes is the format type etc then can easily assign */
    /* for sonata the value should be 00 01 00 03 00 f0 END */
    return 1;
}

MidiEvent *processByte(ParserFSM *fsm, unsigned char eventByte, FILE *file, unsigned int *bytesRead,
                       unsigned int deltaTime, unsigned int currentAbsoluteTime)
// fread the byte here
{
    unsigned char metaType;
    unsigned int additionalBytesRead;
    unsigned int metaLength = 0;
    MidiEvent *result = NULL;

    switch (fsm->currentState)
    {
    case InitialState:
        fsm->currentState = eventByte;
        break;
    case MetaEventState:
        printf("hit a metaData\n");
        if (fread(&metaType, 1, 1, file) != 1)
        {
            break;
        }
        additionalBytesRead = 0;
        *bytesRead += 1;
        metaLength = readVariableLengthQuantity(file, &additionalBytesRead);
        *bytesRead += additionalBytesRead;
        result = processMetaEvent(metaType, &bytesRead, metaLength, file, deltaTime, currentAbsoluteTime);
        if (result == NULL)
        {
            printf("setting to failure state");
            fsm->currentState = ParseFailureState;
        }
        else
        {
            fsm->currentState = InitialState;
        }
        break;
    case SysexEventState1:
    case SysexEventState2:
        // System Exclusive Events
        // Handle System Exclusive event
        printf("hit a Sysex event\n");
        additionalBytesRead = 0;
        metaLength = readVariableLengthQuantity(file, &additionalBytesRead);
        *bytesRead += additionalBytesRead;
        fseek(file, metaLength, SEEK_CUR); // Skipping the Sysex event data
        *bytesRead += metaLength;
        fsm->currentState = InitialState;
        break;
    case ParseFailureState:
    case EndOfFileState:
    case EndTrackState:
        break;
    default:
        // MIDI event
        result = processMidiEvent(eventByte, &bytesRead, file, deltaTime, currentAbsoluteTime);
        printf("assign event 2\n");
        fsm->currentState = InitialState;
        break;
    }
    // create or return event
    printf("returning event 2\n");
    return result;
}

MidiEvent *processMidiEvent(unsigned char eventByte, unsigned int *bytesRead,
                            FILE *file, unsigned int deltaTime, unsigned int currentAbsoluteTime)
{
    unsigned char miditrackType;
    unsigned char param1;
    unsigned char param2;
    unsigned char programSelected;
    printf("PROCESS MIDI EVENT");
    miditrackType = eventByte & 0xF0;
readMidiTrackEventAgain:
    if (miditrackType == 0x80) // Note Off Event
    {
        unsigned char byte;
        if (fread(&byte, 1, 1, file) != 1)
        {
            return NULL; // Handle read error or EOF
        }
        param1 = byte;
        if (fread(&byte, 1, 1, file) != 1)
        {
            return NULL; // Handle read error or EOF
        }
        param2 = byte;
        *bytesRead += 2;
        // notesCount += 1;
    }
    else if (miditrackType == 0x90) // Note On Event
    {
        unsigned char byte;
        if (fread(&byte, 1, 1, file) != 1)
        {
            return NULL; // Handle read error or EOF
        }
        param1 = byte;
        if (fread(&byte, 1, 1, file) != 1)
        {
            return NULL; // Handle read error or EOF
        }
        param2 = byte; // velocity, speed of action of the note i.e. loudness, unused for music xml
        *bytesRead += 2;
        // notesCount += 1;
    }
    else if (miditrackType == 0xA0)
    {
        printf("Note Aftertouch Event\n");
    }
    else if (miditrackType == 0xB0)
    {
        unsigned char byte;
        if (fread(&byte, 1, 1, file) != 1)
        {
            return NULL; // Handle read error or EOF
        }
        param1 = byte; // type of controller
        if (fread(&byte, 1, 1, file) != 1)
        {
            return NULL; // Handle read error or EOF
        }
        param2 = byte; // value to set the controller to, 1 byte 0-127 | 0 | < 63 is off, >= 64 is on
        *bytesRead += 2;
    }
    else if (miditrackType == 0xC0)
    {
        printf("Program Change Event\n");
        unsigned char byte;
        if (fread(&byte, 1, 1, file) != 1)
        {
            return NULL; // Handle read error or EOF
        }
        printf("fread\n");
        programSelected = byte;
        param1 = programSelected;
        *bytesRead += 1;
    }
    else if (miditrackType == 0xD0)
    {
        printf("Channel Aftertouch Event\n");
    }
    else if (miditrackType == 0xE0)
    {
        printf("Pitch Bend Aftertouch Event\n");
    }
    else
    {
        printf("Unknown %02x, running status\n", eventByte);
        miditrackType = runningStatus; // set track as running status
        fseek(file, -1, SEEK_CUR);     // rewind fread one byte to read the correct byte
        *bytesRead -= 1;
        goto readMidiTrackEventAgain;
        // running status in effect
    }
    printf("returning event\n");
    return createMidiEvent(deltaTime, currentAbsoluteTime, eventByte, eventByte,
                           keySigData, timeSigData, instrumentNameData, param1, param2, NULL);
}

MidiEvent *processMetaEvent(unsigned char metaType, unsigned int *bytesRead,
                            unsigned int metaLength, FILE *file, unsigned int deltaTime,
                            unsigned int currentAbsoluteTime)
{
    unsigned char tempoData[3];
    if (metaType == 0x04)
    { // Instrument Name
        printf("found InstrumentData\n");
        // Make sure you have enough space in instrumentNameData, or dynamically allocate based on metaLength
        if (metaLength > sizeof(instrumentNameData) - 1)
        {
            // Handle error: metaLength is too large
            fseek(file, metaLength, SEEK_CUR); // Skipping
            *bytesRead += metaLength;
        }
        else
        {
            if (fread(instrumentNameData, 1, metaLength, file) != metaLength)
            {
                return NULL;
            }
            instrumentNameData[metaLength] = '\0'; // Null-terminate the string
            printf("Instrument Name: %s\n", instrumentNameData);
            *bytesRead += metaLength;
        }
    }
    else if (metaType == 0x51)
    { // tempo setting
        printf("found tempoData\n");
        for (int i = 0; i < 3; ++i)
        {
            unsigned char byte;
            if (fread(&byte, 1, 1, file) != 1)
            {
                return NULL; // Handle read error or EOF
            }
            tempoData[i] = byte;
        }
        *bytesRead += 3;
    }
    else if (metaType == 0x58)
    { // Time signature
        printf("found timeSigdata\n");
        for (int i = 0; i < 4; ++i)
        {
            unsigned char byte;
            if (fread(&byte, 1, 1, file) != 1)
            {
                break; // Handle read error or EOF
            }
            // printf("Byte %d value: %u\n", i, byte); // Print the current byte value
            timeSigData[i] = byte;
        }
        *bytesRead += 4; // Update the bytesRead count
    }
    else if (metaType == 0x59)
    {
        printf("found keySigData\n");
        for (int i = 0; i < 2; ++i)
        {
            unsigned char byte;
            if (fread(&byte, 1, 1, file) != 1)
            {
                return NULL; // Handle read error or EOF
            }
            printf("Byte %d value: %u\n", i, byte); // Print the current byte value
            keySigData[i] = byte;
        }
        *bytesRead += 2;
    }
    else if (metaType == 0x2F)
    { // end of track
        printf("found the end of track\n");
        // this will break the loop and return the value at the bottom
        return NULL;
    }
    else
    {
        // Skip other meta event data
        printf("Unknown metadata type 0x%X encountered. Meta length: %u\n", metaType, metaLength);
        if (fseek(file, metaLength, SEEK_CUR) != 0)
        {
            fprintf(stderr, "Error: Failed to seek past unknown metadata of length %u at bytesRead %u.\n", metaLength, bytesRead);
            return NULL;
        }
        else
        {
            printf("Successfully skipped unknown metadata of length %u.\n", metaLength);
        }
        *bytesRead += metaLength;
    }
    MidiEvent *currentEvent = createMidiEvent(deltaTime, currentAbsoluteTime, 0xFF, metaType,
                                              keySigData, timeSigData, instrumentNameData, NULL, NULL, NULL);
    return currentEvent;
}

// ParseFailureState,
//     InitialState,
//     MetaEventState,
//     SysexEventState,
//     MidiEventState,
//     EndTrackState,
//     EndOfFileState

MidiEvent *readMidiTrackEvents(FILE *file, unsigned int trackLength)
{
    printf("readMidiTrackEvents called\n");
    MidiEvent *firstEvent = NULL, *lastEvent = NULL;
    unsigned int bytesRead = 0; // Keep track of bytes read within this track
    unsigned int deltaTime;
    unsigned int currentAbsoluteTime = 0;
    unsigned char eventByte;
    unsigned int additionalBytesRead = 0;

    // For meta data
    unsigned char metaType;
    unsigned int metaLength = 0;

    // For track data

    unsigned int miditrackLength;
    unsigned char miditrackType;
    unsigned char programSelected;
    unsigned char param1;
    unsigned char param2;

    // init fsm
    ParserFSM fsm2;
    initParserFSM(&fsm2, InitialState);
    MidiEvent *currentEvent = NULL;
    instrumentNameData[0] = NULL;

    // temp
    int i = 0;
    int notesCount = 0;
    while (bytesRead < trackLength)
    { // Loop until end of file or track end
        // Step 1: Read Delta Time
        additionalBytesRead = 0;
        deltaTime = readVariableLengthQuantity(file, &additionalBytesRead);
        currentAbsoluteTime += deltaTime; // Accumulate delta times into absolute time

        // printf("Delta Time: %u, Bytes Consumed: %u\n", deltaTime, additionalBytesRead);
        bytesRead += additionalBytesRead;

        // Step 2
        if (fread(&eventByte, 1, 1, file) != 1)
        {
            break;
        } // Error or EOF
        bytesRead += 1;
        /* Feed eventByte and file into fsm */
        runningStatus = eventByte;
        // Set Initial State to the event State
        processByte(&fsm2, eventByte, file, &bytesRead, deltaTime, currentAbsoluteTime);
        // Process the state
        currentEvent = processByte(&fsm2, eventByte, file, &bytesRead, deltaTime, currentAbsoluteTime);
        // MIDI event
        printf("wqeufnowfc:\n");
        printf("%d\n", fsm2.currentState);

        if (fsm2.currentState == ParseFailureState || fsm2.currentState == EndOfFileState || fsm2.currentState == EndTrackState)
        {
            printf("State Failure: ending processByte");
            break;
        }

        // Handle MIDI event, considering running status if applicable
        // read the MSB to see if its a new midi event, else use the running status
        // store to running status if new event do Note off and Note on
        // NOTE: note on with vel 0 is turn off

        // create currentEvent based on data stored

        // MidiEvent *currentEvent = createMidiEvent(deltaTime, currentAbsoluteTime, eventByte, metaType,
        //   keySigData, timeSigData, instrumentNameData, param1, param2, NULL);
        if (currentEvent == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for MidiEvent.\n");
            break;
        }

        if (!firstEvent)
        {
            firstEvent = currentEvent;
            lastEvent = currentEvent;
        }
        else
        {
            printf("assigning next event\n");
            lastEvent->next = currentEvent;
            lastEvent = currentEvent;
        }
        currentEvent->next = NULL; // Ensures the newly added event points to NULL as its next.
    };
    printf("Expected Track Length: %u, Bytes Read: %u\n", trackLength, bytesRead);
    // return a pointer to the first MidiEvent, MidiTrack is a list of MidiEvents
    printf("Total notes in track: %i\n", notesCount);
    return firstEvent;
}

MidiEvent *createMidiEvent(unsigned int deltaTime, unsigned int absoluteTime, unsigned char eventType, unsigned char metaType, unsigned char *keySignature, unsigned char *timeSignature, char *instrument,
                           unsigned char param1, unsigned char param2, struct MidiEvent *next)
{
    MidiEvent *midiEvent = malloc(sizeof(MidiEvent));
    midiEvent->deltaTime = deltaTime;
    midiEvent->absoluteTime = absoluteTime;
    midiEvent->eventType = eventType;
    midiEvent->metaType = metaType;
    midiEvent->instrument = instrument;
    midiEvent->keySignature[0] = keySignature[0];
    midiEvent->keySignature[1] = keySignature[1];
    midiEvent->timeSignature[0] = timeSignature[0];
    midiEvent->timeSignature[1] = timeSignature[1];
    midiEvent->timeSignature[2] = timeSignature[2];
    midiEvent->timeSignature[3] = timeSignature[3];
    midiEvent->param1 = param1;
    midiEvent->param2 = param2;
    midiEvent->next = next;
    return midiEvent;
}

unsigned int readVariableLengthQuantity(FILE *file, unsigned int *bytesRead)
{
    unsigned int value = 0;
    unsigned char c;
    if (fread(&c, 1, 1, file) == 1)
    {
        *bytesRead += 1;  // Update bytesRead for the first byte read
        value = c & 0x7F; // 0x7F = 01111111, and operator with 'c' will remove c's MSB
        while (c & 0x80)
        { // 0x80 = 10000000, if c MSB is 1, then it will return "1000000", Which is 1
            if (fread(&c, 1, 1, file) != 1)
                break;
            *bytesRead += 1; // Update bytesRead for each subsequent byte read
            value = (value << 7) | (c & 0x7F);
        }
    }
    return value;
}

unsigned int
checkMTRK(FILE *file, unsigned int *trackLength)
{
    printf("checkMTRK called\n");
    unsigned char mtrk[] = {0x4d, 0x54, 0x72, 0x6b}; // 'MTrk'
    unsigned char buffer[4];

    // Read and check the track header
    if (fread(buffer, 1, 4, file) != 4)
    {
        printf("buffer !=4\n");
        return 0;
    }
    for (int i = 0; i < 4; ++i)
    {
        if (buffer[i] != mtrk[i])
        {
            printf("buffer[i] != mtrk[i]\n");
            return 0;
        }
    }

    // Read the track length
    unsigned char lengthBuffer[4];
    if (fread(lengthBuffer, 1, 4, file) != 4)
    {
        printf("lengthBuffer != 4\n");
        return 0;
    }

    // Convert to unsigned int
    *trackLength = (lengthBuffer[0] << 24) | (lengthBuffer[1] << 16) | (lengthBuffer[2] << 8) | lengthBuffer[3];
    return 1;
}

MidiFile *readMidiFile(const char *filename)
{
    // MidiEvent *tempEvent = NULL;
    printf("reading midifile\n");
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("it is not file\n");
        return NULL;
    }

    MidiFile *midiFile = malloc(sizeof(MidiFile));
    if (!midiFile)
    {
        printf("error allocating midifile size\n");
        fclose(file);
        return NULL;
    }

    // Read header
    if (!readMidiHeader(file, &midiFile->header))
    {
        // Handle error or unsupported format
        printf("readMidiHeader error\n");
        fclose(file);
        free(midiFile);
        printf("Error in midi header, exiting.\n");
        return NULL;
    }

    // Alloc all tracks,
    midiFile->tracks = malloc(sizeof(MidiTrack) * midiFile->header.numberOfTracks);
    for (int i = 0; i < midiFile->header.numberOfTracks; ++i)
    {
        printf("Processing Track %d\n", i);
        // 1) Read and validate the track header here (e.g., ensure it starts with 'MTrk'), if it is, read it
        unsigned int trackLength = 0;
        if (checkMTRK(file, &trackLength))
        {
            printf("Reading tracks..\n");
            midiFile->tracks[i].events = readMidiTrackEvents(file, trackLength);
            printf("track[i] = %d completed\n\n", i);
        }
        else
        {
            // Error handling
            printf("Failed to validate track start marker = %u\n", i);
        }
    }

    fclose(file);
    printf("returning midiFile\n");
    return midiFile;
}