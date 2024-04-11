#include <stdio.h>
#include "main.h"

typedef struct
{
    char *noteType;
    char *pitchType;
    int isChord;
    int duration;
    int midiNoteNumber;
    struct MidiNote *next; // move to next event with midiEvent.next
} MidiNote;

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

int noteOff(MidiEvent*event){
    if(event->eventType==0x80 || (event->eventType == 0x90 && event->param2<0)){
        return 1;
    }
    else{
        return 0;
    }
}

int findNoteDuration(MidiEvent*event, int midiNoteNumber){
    int duration = 0;
    while(event!=NULL){
        if(noteOff(event)&&(event->param1==midiNoteNumber)){
            duration+=event->deltaTime;
        }
        event = event->next;
    }
    return duration;
}

MidiNote *createMidiNote(MidiFile *midiFile)
{
    int totalNoteCount = 0;
    int maxNotes = 3000; // This should be dynamically calculated based on the actual MIDI file content
    MidiNote *midiNotes = malloc(maxNotes * sizeof(MidiNote));
    int restTime = 0;
    int threshold = midiFile->header.timeDivision / 16;
    if (midiNotes == NULL)
    {
        // Handle memory allocation error
        return NULL;
    }
    for (int trackIndex = 0; trackIndex < midiFile->header.numberOfTracks; trackIndex++)
    {
        printf("current trackIndex = %d\n", trackIndex);
        MidiEvent *event = midiFile->tracks[trackIndex].events;

        while (event != NULL && totalNoteCount < maxNotes)
        {
            // if (event->eventType != 0xFF && event->eventType != 0xF0 && event->eventType != 0xF7 && event->eventType >= 0x80)
            if(event->eventType >= 0x80 && event->eventType <= 0x90)
            {
                if (restTime >= threshold && restTime != 0)
                {
                    midiNotes[totalNoteCount].noteType = "rest";
                }
                if (noteOn(event))
                {
                    restTime = 0;
                    midiNotes[totalNoteCount].midiNoteNumber = event->param1;
                    midiNotes[totalNoteCount].noteType = "pitch";
                    midiNotes[totalNoteCount].duration = findNoteDuration(event, event->param1);

                    // find chord
                    if (noteOn(event->next))
                    {
                        midiNotes[totalNoteCount].isChord = 1;
                    }
                    else
                    {
                        midiNotes[totalNoteCount].isChord = 0;
                    }
                }
                totalNoteCount++;
            }
            event = event->next;
        }
        printf("finished trackIndex = %d with noteCount = %d\n\n", trackIndex,totalNoteCount);
    }
    return midiNotes;
}
