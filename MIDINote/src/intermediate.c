#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "main.h"

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

int findNoteDuration(MidiEvent *startEvent)
{
    MidiEvent *event = startEvent->next;
    while (event != NULL)
    {
        if ((event->eventType == 0x80 || (event->eventType == 0x90 && event->param2 == 0)) && event->param1 == startEvent->param1)
        {
            return event->absoluteTime - startEvent->absoluteTime;
        }
        event = event->next;
    }
    return -1; // Indicate that no matching Note Off event was found
}

MidiNote *createMidiNoteForTrack(MidiFile *midiFile, MidiTrack track, int maxNotes)
{
    MidiNote *firstNote = NULL, *lastNote = NULL;
    MidiEvent *event = track.events;
    unsigned int totalNoteCount = 0;
    unsigned int lastAbsoluteTime = 0; // To track the start time of the last note
    int isFirstNoteInChord = 1;        // Flag to check if the note is the first in a chord

    while (event != NULL && totalNoteCount < maxNotes)
    {
        if (event->eventType == 0x90 && event->param2 > 0)
        { // Note On event with non-zero velocity
            int duration = findNoteDuration(event);
            if (duration > 0)
            { // Ensure valid duration
                MidiNote *currentNote = (MidiNote *)malloc(sizeof(MidiNote));
                if (!currentNote)
                {
                    fprintf(stderr, "Error allocating memory for MidiNote\n");
                    // Cleanup and exit or return what we have so far
                    return firstNote;
                }

                // Assigning properties to the current note
                currentNote->midiNoteNumber = event->param1;
                currentNote->duration = duration;
                currentNote->isChord = 0; // Default value

                // Assigning properties based on event
                currentNote->beats = event->timeSignature[0];
                currentNote->beatsType = pow(2, event->timeSignature[1]);
                currentNote->keySignature = event->keySignature[0]; // Assuming major/minor only
                currentNote->noteType = "pitch";                    // Defaulting to "pitch", adjust as needed
                // Assuming 'instrument' field is a part of MidiTrack or a global variable
                currentNote->instrument = event->instrument;

                // Check if this note is part of a chord
                if (event->absoluteTime == lastAbsoluteTime && !isFirstNoteInChord)
                {
                    currentNote->isChord = 1; // This is part of a chord
                }
                else
                {
                    lastAbsoluteTime = event->absoluteTime; // Update last absolute time
                    isFirstNoteInChord = 0;                 // Any subsequent note at this time is part of a chord
                }

                // Linking the note into the list
                if (!firstNote)
                {
                    firstNote = currentNote;
                }
                else
                {
                    lastNote->next = currentNote;
                }
                lastNote = currentNote;
                currentNote->next = NULL;

                totalNoteCount++;
            }
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
                    durationValue = findNoteDuration(event);
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