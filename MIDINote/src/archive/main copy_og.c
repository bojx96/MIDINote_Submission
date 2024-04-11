#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "main.h"

void createHeaders(FILE *file)
{
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(file, "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 3.1 Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">\n");
    fprintf(file, "<score-partwise version=\"3.1\">\n");
}

void closeHeaders(FILE *file)
{
    fprintf(file, "</score-partwise>\n");
    fclose(file);
}

void createPartList(FILE *file, int numberOfParts)
{
    // number of Parts is base on number of tracks
    // this can be called from MidiHeader
    int i;
    fprintf(file, "  <part-list>\n");
    for (i = 0; i < numberOfParts; i++)
    {
        fprintf(file, "    <score-part id=\"P%d\">\n", i);
        fprintf(file, "      <part-name>Music</part-name>\n");
        fprintf(file, "    </score-part>\n");
    }
    fprintf(file, "  </part-list>\n");
}

void createPart(FILE *file, int *currentPartNumber, int division, int fifths, int beats, int beatsType, char *instrumentType)
{
    // division can be found at Header Chunk's last 2 bytes, this is found in header
    // fifths can be found inside key signature FF 59 02 sf mi, this is found in MidiEvent.keySignature
    // clefs.. can't find in midi, its actually base on instrument, have G, C ,F, exist on 5 lines, MidiEvent.instrument
    char *violin = "violin";
    char *guitar = "guitar";
    char *bass = "bass";
    char *piano = "piano";
    char *viola = "viola";

    char clefSign;
    int clefLine;

    if (instrumentType == NULL)
    {
        // default clef if it doesn't exist
        clefSign = 'G';
        clefLine = 2;
    }
    else if (strcmp(instrumentType, violin) == 0)
    {
        clefSign = 'G';
        clefLine = 2;
    }
    else if (strcmp(instrumentType, guitar) == 0)
    {
        clefSign = 'G';
        clefLine = 2;
    }
    else if (strcmp(instrumentType, bass) == 0)
    {
        clefSign = 'F';
        clefLine = 4;
    }
    else if (strcmp(instrumentType, viola) == 0)
    {
        clefSign = 'C';
        clefLine = 3;
    }

    fprintf(file, "  <part id=\"P%d\">\n", *currentPartNumber);
    fprintf(file, "    <measure number=\"%d\">\n", *currentPartNumber);
    fprintf(file, "      <attributes>\n");
    fprintf(file, "        <divisions>%d</divisions>\n", division);
    fprintf(file, "        <key>\n");
    fprintf(file, "          <fifths>%d</fifths>\n", fifths);
    fprintf(file, "        </key>\n");
    fprintf(file, "        <time>\n");
    fprintf(file, "          <beats>%d</beats>\n", beats);
    fprintf(file, "          <beat-type>%d</beat-type>\n", beatsType);
    fprintf(file, "        </time>\n");
    fprintf(file, "        <clef>\n");
    fprintf(file, "          <sign>%c</sign>\n", clefSign);
    fprintf(file, "          <line>%d</line>\n", clefLine);
    fprintf(file, "        </clef>\n");
    fprintf(file, "      </attributes>\n");
}

void closePart(FILE *file)
{
    fprintf(file, "    </measure>\n");
    fprintf(file, "  </part>\n");
}

void midiNoteToStepAndOctave(int midiNoteNumber, int *octave, char **step)
{
    char *notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    *octave = midiNoteNumber / 12 - 1;
    *step = notes[midiNoteNumber % 12];
}

void createPitch(FILE *file, int midiNoteNumber)
{
    char *step; // to be assigned later by midiNoteToStepAndOctave
    int octave; // to be assigned later by midiNoteToStepAndOctave
    midiNoteToStepAndOctave(midiNoteNumber, &octave, &step);

    fprintf(file, "        <pitch>\n");
    fprintf(file, "          <step>%c</step>\n", *step);
    fprintf(file, "          <octave>%d</octave>\n", octave);
    fprintf(file, "        </pitch>\n");
}

void createNote(FILE *file, MidiNote midiNote)
{
    char *rest = "rest";

    // printf("inside createNote, midiNote.isChord = %d \n",midiNote.isChord);
    // printf("inside createNote, midiNote.noteType = %s \n",midiNote.noteType);
    // printf("inside createNote, midiNote.duration = %d \n",midiNote.duration);

    fprintf(file, "      <note>\n");
    if (midiNote.isChord == 1)
    {
        fprintf(file, "       <chord/>\n");
    }

    if (strcmp(midiNote.noteType, rest) == 0)
    {
        fprintf(file, "        <rest measure=\"yes\"/>\n");
    }
    else
    {
        // handle pitch here
        createPitch(file, midiNote.midiNoteNumber);
    }
    fprintf(file, "        <duration>%d</duration>\n", midiNote.duration);
    fprintf(file, "      </note>\n");
}

int noteOn(MidiEvent *event)
{
    if (event == NULL)
    {
        printf("Error: event is NULL.\n");
        return 0; // Or handle the error
    }
    if (event->eventType != 0 && event->eventType == 0x90 && event->param2 > 0)
    {

        return 1;
    }
    else
    {
        return 0;
    }
}

int noteOff(MidiEvent *event)
{
    if (event->eventType == 0x80 || (event->eventType == 0x90 && event->param2 < 0))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int findNoteDuration(MidiEvent *event, int midiNoteNumber)
{
    int duration = 0;
    while (event != NULL)
    {
        if (noteOff(event) && (event->param1 == midiNoteNumber))
        {
            duration += event->deltaTime;
        }
        event = event->next;
    }
    return duration;
}

MidiNote *createMidiNoteForTrack(MidiFile *midiFile, MidiTrack track, int maxNotes)
{
    int totalNoteCount = 0;
    MidiNote *firstNote = NULL, *lastNote = NULL;
    int restTime = 0;
    int threshold = midiFile->header.timeDivision / 16;

    // Initialize all local variables to safe default values
    MidiEvent *event = track.events;
    int beatsValue = 0;
    int beatsTypeValue = 1; // Defaulting to 1 to avoid division by zero or uninitialized use
    char *noteTypeString = NULL;
    char durationValue = 0;
    int keySignatureValue = 0;

    while (event != NULL && totalNoteCount < maxNotes)
    {
        int isChordValue = 0;
        int midiNoteNumberValue = 0; // Initialize to 0 or a safe default

        if (event->eventType == 0xFF)
        {
            beatsValue = event->timeSignature[0];
            beatsTypeValue = pow(2, event->timeSignature[1]);
        }

        if (event->eventType >= 0x80 && event->eventType <= 0x90)
        {
            noteTypeString = (event->eventType == 0x90) ? "pitch" : "rest";

            if (noteOn(event))
            {
                restTime = 0;
                midiNoteNumberValue = event->param1;
                durationValue = findNoteDuration(event, event->param1);
                keySignatureValue = (int)event->keySignature[0];
                printf("keySignatureValue from event->keySignature[0] = %d\n", event->keySignature[0]);
                // Determine chord
                isChordValue = noteOn(event->next) ? 1 : 0;
            }
            else
            {
                restTime += event->deltaTime;
                if (restTime >= threshold)
                {
                    durationValue = 64;
                }
            }

            MidiNote *currentNote = (MidiNote *)malloc(sizeof(MidiNote));
            if (currentNote == NULL)
            {
                fprintf(stderr, "Error allocating memory for MidiNote\n");
                // Consider cleanup here (freeing previously allocated notes)
                return NULL;
            }
            // Ensure proper initialization of all fields
            *currentNote = (MidiNote){.beats = beatsValue, .beatsType = beatsTypeValue, .duration = durationValue, .isChord = isChordValue, .keySignature = keySignatureValue, .midiNoteNumber = midiNoteNumberValue, .noteType = noteTypeString, .next = NULL};
            printf("currentNote keySignatureValue = %d\n", keySignatureValue);
            if (!firstNote)
            {
                firstNote = lastNote = currentNote;
            }
            else
            {
                lastNote->next = currentNote;
                lastNote = currentNote;
            }

            totalNoteCount++;
        }
        event = event->next;
    }
    return firstNote;
}

MidiNote *createMidiNote(MidiFile *midiFile, int maxNotes)
{
    int totalNoteCount = 0;
    MidiNote *firstNote = NULL, *lastNote = NULL;
    int restTime = 0;
    int threshold = midiFile->header.timeDivision / 16;

    for (int trackIndex = 0; trackIndex < midiFile->header.numberOfTracks; trackIndex++)
    {
        printf("current trackIndex = %d\n", trackIndex);
        MidiEvent *event = midiFile->tracks[trackIndex].events;

        while (event != NULL && totalNoteCount < maxNotes)
        {
            int isChordValue = 0;
            int midiNoteNumberValue;
            char *noteTypeString;
            char *instrumentString;
            char durationValue = 0;
            int keySignatureValue;
            int beatsValue;
            int beatsTypeValue;

            if (event->eventType == 0xFF)
            { // this is meta event..
                beatsValue = event->timeSignature[0];
                beatsTypeValue = pow(2, event->timeSignature[1]);
                if (event->instrument == NULL)
                {
                    instrumentString = "";
                }
                else
                {
                    instrumentString = event->instrument;
                }
            }

            if (event->eventType >= 0x80 && event->eventType <= 0x90)
            {
                if (restTime >= threshold && restTime != 0)
                {
                    noteTypeString = "rest";
                    durationValue = 64;
                    // midiNotes[totalNoteCount].noteType = "rest";
                }
                if (noteOn(event))
                {
                    restTime = 0;
                    // midiNotes[totalNoteCount].midiNoteNumber = event->param1;
                    // midiNotes[totalNoteCount].noteType = "pitch";
                    // midiNotes[totalNoteCount].duration = findNoteDuration(event, event->param1);
                    midiNoteNumberValue = event->param1;
                    noteTypeString = "pitch";
                    durationValue = findNoteDuration(event, event->param1);
                    keySignatureValue = (int)event->keySignature[0];

                    // find chord
                    if (noteOn(event->next))
                    {
                        // midiNotes[totalNoteCount].isChord = 1;
                        isChordValue = 1;
                    }
                    else
                    {
                        // midiNotes[totalNoteCount].isChord = 0;
                        isChordValue = 0;
                    }
                }
                MidiNote *currentNote = (MidiNote *)malloc(sizeof(MidiNote));
                if (currentNote == NULL)
                {
                    return NULL;
                }
                currentNote->beats = beatsValue;
                currentNote->beatsType = beatsTypeValue;
                currentNote->duration = durationValue;
                currentNote->isChord = isChordValue;
                currentNote->keySignature = keySignatureValue;
                currentNote->midiNoteNumber = midiNoteNumberValue;
                currentNote->noteType = noteTypeString;

                if (!firstNote)
                {
                    firstNote = currentNote;
                    lastNote = currentNote;
                }
                else
                {
                    lastNote->next = currentNote;
                    lastNote = currentNote;
                }
                currentNote->next = NULL;
                totalNoteCount++;
            }
            event = event->next;
        }
        printf("finished trackIndex = %d with noteCount = %d\n\n", trackIndex, totalNoteCount);
    }
    return firstNote;
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
MidiEvent *readMidiTrackEvents(FILE *file, unsigned int trackLength)
{
    printf("readMidiTrackEvents called\n");
    MidiEvent *firstEvent = NULL, *lastEvent = NULL;
    unsigned int bytesRead = 0; // Keep track of bytes read within this track
    unsigned int deltaTime;
    unsigned char eventByte;
    unsigned int additionalBytesRead = 0;

    // For meta data
    unsigned char metaType;
    unsigned int metaLength = 0;
    unsigned char tempoData[3];
    unsigned char timeSigData[4];

    // For track data
    unsigned char runningStatus;
    unsigned int miditrackLength;
    unsigned char miditrackType;
    unsigned char programSelected;
    unsigned char param1;
    unsigned char param2;

    unsigned char keySigData[2];
    unsigned char instrumentNameData[100];

    // temp
    int i = 0;
    int notesCount = 0;
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
                // this will break the loop and return the value at the bottom
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
            miditrackType = eventByte & 0xF0;
        readMidiTrackEventAgain:
            if (miditrackType == 0x80) // Note Off Event
            {
                unsigned char byte;
                if (fread(&byte, 1, 1, file) != 1)
                {
                    break; // Handle read error or EOF
                }
                param1 = byte;
                if (fread(&byte, 1, 1, file) != 1)
                {
                    break; // Handle read error or EOF
                }
                param2 = byte;
                bytesRead += 2;
                notesCount += 1;
            }
            else if (miditrackType == 0x90) // Note On Event
            {
                unsigned char byte;
                if (fread(&byte, 1, 1, file) != 1)
                {
                    break; // Handle read error or EOF
                }
                param1 = byte;
                if (fread(&byte, 1, 1, file) != 1)
                {
                    break; // Handle read error or EOF
                }
                param2 = byte; // velocity, speed of action of the note i.e. loudness, unused for music xml
                bytesRead += 2;
                notesCount += 1;
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
                    break; // Handle read error or EOF
                }
                param1 = byte; // type of controller
                if (fread(&byte, 1, 1, file) != 1)
                {
                    break; // Handle read error or EOF
                }
                param2 = byte; // value to set the controller to, 1 byte 0-127 | 0 | < 63 is off, >= 64 is on
                bytesRead += 2;
            }
            else if (miditrackType == 0xC0)
            {
                printf("Program Change Event\n");
                unsigned char byte;
                if (fread(&byte, 1, 1, file) != 1)
                {
                    break; // Handle read error or EOF
                }
                programSelected = byte;
                param1 = programSelected;
                bytesRead += 1;
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
                bytesRead -= 1;
                goto readMidiTrackEventAgain;
                // running status in effect
            }

            // Handle MIDI event, considering running status if applicable
            // read the MSB to see if its a new midi event, else use the running status
            // store to running status if new event do Note off and Note on
            // NOTE: note on with vel 0 is turn off

            runningStatus = miditrackType;
        }
        // create currentEvent based on data stored

        MidiEvent *currentEvent = createMidiEvent(deltaTime, eventByte, metaType,
                                                  keySigData, timeSigData, instrumentNameData, param1, param2, NULL);
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
            lastEvent->next = currentEvent;
            lastEvent = currentEvent;
        }
        currentEvent->next = NULL; // Ensures the newly added event points to NULL as its next.
    };
    printf("Expected Track Length: %u, Bytes Read: %u\n", trackLength, bytesRead);
    // return a pointer to the first MidiEvent, MidiTrack is a list of MidiEvents
    printf("Total notes in track: %i\n", notesCount);
    if (lastEvent != NULL)
    {
        lastEvent->next = NULL; // Properly terminate the current track's event list
    }
    return firstEvent;
}

MidiEvent *createMidiEvent(unsigned int deltaTime, unsigned char eventType, unsigned char metaType, unsigned char *keySignature, unsigned char *timeSignature, char *instrument,
                           unsigned char param1, unsigned char param2, struct MidiEvent *next)
{
    MidiEvent *midiEvent = malloc(sizeof(MidiEvent));
    midiEvent->deltaTime = deltaTime;
    midiEvent->eventType = eventType;
    midiEvent->metaType = metaType;
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

int main(int argc, char **argv)
{
    unsigned char buffer[100];
    FILE *midiFile;

    size_t bytesRead, i;
    struct MidiHeader *midiheader = malloc(sizeof(struct MidiHeader));

    printf("--- test readmidifile ---\n");
    MidiFile *testMidi = malloc(sizeof(struct MidiFile));
    testMidi = readMidiFile(argv[1]);
    int maxNotes = 3000;
    // MidiNote *midiNotes = createMidiNote(testMidi, maxNotes);
    // printf("midiNotes = %p\n", midiNotes);
    // printf("midiNotes next = %p\n", midiNotes->next);

    printf("--- Creating MusicXML file ---\n");
    FILE *xmlFile = fopen("output.xml", "w");
    if (!xmlFile)
    {
        fprintf(stderr, "Failed to create MusicXML file.\n");
        return 1;
    }

    createHeaders(xmlFile); // Start of the XML file

    createPartList(xmlFile, testMidi->header.numberOfTracks);
    int currentPartNumber = 0;

    // for (int j = 0; j < testMidi->header.numberOfTracks; j++)
    for (int j = 0; j < testMidi->header.numberOfTracks; j++)
    {
        printf("current for loop interation = %d\n", j);
        MidiNote *trackNotes = createMidiNoteForTrack(testMidi, testMidi->tracks[j], maxNotes);
        printf("trackNotes address = %p\n", trackNotes);
        int counter = 0;
        if (trackNotes != NULL)
        {
            createPart(xmlFile, &j, testMidi->header.timeDivision, trackNotes->keySignature, trackNotes->beats, trackNotes->beatsType, trackNotes->instrument);
            while (trackNotes->next != NULL && counter < 50)
            {
                printf("current track iteration value %d, address = %p\n", counter, trackNotes);
                createNote(xmlFile, *trackNotes);
                trackNotes = trackNotes->next;
                counter++;
            }
            printf("closing part\n");
            closePart(xmlFile);
        }
    }

    closeHeaders(xmlFile);
    fclose(midiFile);
}
