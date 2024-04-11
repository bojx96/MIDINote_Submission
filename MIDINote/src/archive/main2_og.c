#include <stdio.h>
#include <stdlib.h>
#include "main.h"

typedef enum
{
    InitialState,
    HeaderState,
    TrackStartState,
    EventState,
    MetaEventState,
    SysexEventState,
    MidiEventState,
    EndTrackState,
    EndOfFileState
} ParserState;

typedef struct
{
    ParserState current_state;
    FILE *file;
    MidiFile *midi_file;
    unsigned int track_length;
} FSM;

void update_state(FSM *fsm)
{
    if (fsm->current_state == InitialState)
    {
        handleInitialState(fsm);
    }
    else if (fsm->current_state == HeaderState)
    {
        handleHeaderState(fsm);
    }
    else if (fsm->current_state == TrackStartState)
    {
        handleTrackStartState(fsm);
    }
    else if (fsm->current_state == EventState)
    {
        handleTrackEventState(fsm);
    }
    // else if(fsm->current_state == MetaEventState){}
    // else if(fsm->current_state == SysexEventState){}
    // else if(fsm->current_state == MidiEventState){}
    else if (fsm->current_state == EndTrackState)
    {
    }
    else if (fsm->current_state == EndOfFileState)
    {
    }
}

int initFSM(FSM *fsm, const char *filename)
{
    if (!fsm)
    {
        printf("FSM pointer is null\n");
        return -1;
    }
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("File inputted is not a valid file\n");
        return -2;
    }

    fsm->current_state = InitialState;
    fsm->file = file;
    fsm->track_length = 0;
    return 0; // Success
}

void handleInitialState(FSM *fsm)
{
    MidiFile *midiFile = malloc(sizeof(MidiFile));
    if (!midiFile)
    {
        printf("Error allocating MIDI file size\n");
        fclose(fsm->file);
        fsm->current_state = EndOfFileState;
    }
    fsm->midi_file = midiFile;
    fsm->current_state = HeaderState;
}

void handleHeaderState(FSM *fsm)
{
    if (readMidiHeader(fsm->file, &fsm->midi_file->header))
    {
        printf("MIDI Header successfully read. Proceeding to track reading.\n");
        fsm->current_state = TrackStartState;
    }
    else
    {
        printf("Invalid MIDI header. Exiting.\n");
        fsm->current_state = EndOfFileState;
    }
}

void handleTrackStartState(FSM *fsm)
{
    if (checkMTRK(fsm->file, &fsm->track_length))
    {
        fsm->current_state = EventState;
    }
    else
    {
        fsm->current_state = EndOfFileState;
        printf("Failed to find track start marker.\n");
    }
}

void handleTrackEventState(FSM *fsm)
{
    readMidiTrackEvents(fsm->file, &fsm->track_length);
    // not fully done
}

int readMidiHeader(FILE *file, struct MidiHeader *header)
{
    unsigned char buffer[100];
    unsigned char msb_mask = 0x80;
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
    header->formatType = (((unsigned short)buffer[8]) << 8) | (((unsigned short)buffer[9]) << 0);
    header->numberOfTracks = (((unsigned short)buffer[10]) << 8) | (((unsigned short)buffer[11]) << 0);
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
    return 1;
}
MidiEvent *readMidiTrackEvents(FILE *file, unsigned int trackLength)
{
    printf("readMidiTrackEvents called\n");
    MidiEvent *firstEvent = NULL, *currentEvent = NULL;
    unsigned int bytesRead = 0; // Keep track of bytes read within this track
    unsigned int deltaTime;
    unsigned char eventByte;
    unsigned int additionalBytesRead = 0;

    // For meta data
    unsigned char metaType;
    unsigned int metaLength;
    unsigned char tempoData[3];
    unsigned char timeSigData[4];

    // For track data
    unsigned char runningStatus;
    unsigned int miditrackLength;
    unsigned char miditrackType;

    unsigned char keySigData[2];
    unsigned char instrumentNameData[100];

    // temp
    int i = 0;
    while (bytesRead < trackLength)
    { // Loop until end of file or track end
        // Step 1: Read Delta Time
        additionalBytesRead = 0;
        deltaTime = readVariableLengthQuantity(file, &additionalBytesRead);
        // printf("Delta Time: %u, Bytes Consumed: %u\n", deltaTime, additionalBytesRead);
        bytesRead += additionalBytesRead;

        // Step 2
        if (fread(&eventByte, 1, 1, file) != 1)
        {
            break;
        } // Error or EOF
        bytesRead++;

        if (eventByte == 0xFF)
        { // Meta event
            printf("hit a metaData\n");
            if (fread(&metaType, 1, 1, file) != 1)
            {
                break;
            }
            additionalBytesRead = 0;
            bytesRead++;
            metaLength = readVariableLengthQuantity(file, &additionalBytesRead);
            bytesRead += additionalBytesRead;

            if (metaType == 0x04)
            { // Instrument Name
                printf("found InstrumentData\n");
                // Make sure you have enough space in instrumentNameData, or dynamically allocate based on metaLength
                if (metaLength > sizeof(instrumentNameData) - 1)
                {
                    // Handle error: metaLength is too large
                    fseek(file, metaLength, SEEK_CUR); // Skipping
                    bytesRead += metaLength;
                }
                else
                {
                    if (fread(instrumentNameData, 1, metaLength, file) != metaLength)
                    {
                        break;
                    }
                    instrumentNameData[metaLength] = '\0'; // Null-terminate the string
                    printf("Instrument Name: %s\n", instrumentNameData);
                    bytesRead += metaLength;
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
                        break; // Handle read error or EOF
                    }
                    tempoData[i] = byte;
                }
                bytesRead += 3;
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
                bytesRead += 4; // Update the bytesRead count
            }
            else if (metaType == 0x59)
            {
                printf("found keySigData\n");
                for (int i = 0; i < 2; ++i)
                {
                    unsigned char byte;
                    if (fread(&byte, 1, 1, file) != 1)
                    {
                        break; // Handle read error or EOF
                    }
                    printf("Byte %d value: %u\n", i, byte); // Print the current byte value
                    keySigData[i] = byte;
                }
                bytesRead += 2;
            }
            else if (metaType == 0x2F)
            { // end of track
                printf("found the end of track\n");
                break;
            }
            else
            {
                // Skip other meta event data
                printf("Unknown metadata type 0x%X encountered. Meta length: %u\n", metaType, metaLength);
                if (fseek(file, metaLength, SEEK_CUR) != 0)
                {
                    fprintf(stderr, "Error: Failed to seek past unknown metadata of length %u at bytesRead %u.\n", metaLength, bytesRead);
                    break;
                }
                else
                {
                    printf("Successfully skipped unknown metadata of length %u.\n", metaLength);
                }
                bytesRead += metaLength;
            }
        }
        else if (eventByte == 0xF0 || eventByte == 0xF7)
        { // System Exclusive Events
            // Handle System Exclusive event
            printf("hit a Sysex event\n");
            metaLength = readVariableLengthQuantity(file, &additionalBytesRead);
            bytesRead += additionalBytesRead;
            fseek(file, metaLength, SEEK_CUR); // Skipping the Sysex event data
            bytesRead += metaLength;
        }
        else
        { // MIDI event
            i++;
            miditrackType = eventByte;
            if (miditrackType & 0xF0 == 0x80)
            {
                if (i < 10)
                    printf("Note Off Event\n");
            }
            else if (miditrackType & 0xF0 == 0x90)
            {
                if (i < 10)
                    printf("Note On Event\n");
            }
            else if (miditrackType & 0xF0 == 0xA0)
            {
                if (i < 10)
                    printf("Note Aftertouch Event\n");
            }
            else if (miditrackType & 0xF0 == 0xB0)
            {
                if (i < 10)
                    printf("Controller Event\n");
            }
            else if (miditrackType & 0xF0 == 0xC0)
            {
                if (i < 10)
                    printf("Program Change Event\n");
            }
            else if (miditrackType & 0xF0 == 0xD0)
            {
                if (i < 10)
                    printf("Channel Aftertouch Event\n");
            }
            else if (miditrackType & 0xF0 == 0xE0)
            {
                if (i < 10)
                    printf("Pitch Bend Aftertouch Event\n");
            }
            else
            {
                if (i < 10)
                    printf("Unknown, running status\n");
                // running status in effect
            }

            // Handle MIDI event, considering running status if applicable
            // read the MSB to see if its a new midi event, else use the running status
            // store to running status if new event do Note off and Note on
            // NOTE: note on with vel 0 is turn off
            if (i < 10)
            {
                printf("MIDI Event: %d %02x \n", i, (eventByte));
            }
        }
        // If firstEvent is NULL, this is the first event
        if (!firstEvent)
        {
            firstEvent = currentEvent; // Initialize list with the first event
        }
    };
    printf("Expected Track Length: %u, Bytes Read: %u\n", trackLength, bytesRead);

    return firstEvent;
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
        printf("Processing Track %d\n", i + 1);
        // 1) Read and validate the track header here (e.g., ensure it starts with 'MTrk'), if it is, read it
        unsigned int trackLength = 0;
        if (checkMTRK(file, &trackLength))
        {
            printf("Reading tracks..\n");
            midiFile->tracks[i].events = readMidiTrackEvents(file, trackLength);
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

int main(int argc, char **argv)
{
    unsigned char buffer[100];
    FILE *midiFile;
    size_t bytesRead, i;
    struct MidiHeader *midiheader = malloc(sizeof(struct MidiHeader));

    if (argc < 2)
    {
        printf("Example: %s <MIDI file>\n", argv[0]);
        return 1;
    }

    midiFile = fopen(argv[1], "rb");
    if (!midiFile)
    {
        perror("Failed to open file");
        return 1; /* TODO: Change to appropriate exit */
    }
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), midiFile)) > 0)
    {
        for (i = 0; i < bytesRead; ++i)
        {
            printf("%02x ", buffer[i]);
        }
        printf("\n");
    }
    /* temp. to remove */
    printf("--- test readmidifile ---\n");
    readMidiFile(argv[1]);

    fclose(midiFile);
}
