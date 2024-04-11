#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

typedef enum
{
    StartState,
    ProcessMidiFileState,
    IntermediateProcessState,
    ConvertXMLState,
    FinalState,
    FailureState,
} MIDINoteState;

typedef struct
{
    MIDINoteState currentState;
    int status;
} MIDINoteFSM;

void initFSM(MIDINoteFSM *fsm, char letter)
{
    fsm->currentState = StartState;
    fsm->status = 0;
}

void processMidi(MIDINoteFSM *fsm, int status)
{
    switch (fsm->currentState)
    {
    case StartState:
        if (status == 1)
        {
            fsm->currentState = ProcessMidiFileState;
            printf("**ProcessMidiFileState**\n");
        }
        else
        {
            fsm->currentState = FailureState;
        }
        break;
    case ProcessMidiFileState:
        if (status == 1)
        {
            fsm->currentState = IntermediateProcessState;
            printf("**IntermediateProcessState**\n");
        }
        else
        {
            fsm->currentState = FailureState;
        }
        break;
    case IntermediateProcessState:
        if (status == 1)
        {
            fsm->currentState = ConvertXMLState;
            printf("**ConvertXMLState**\n");
        }
        else
        {
            fsm->currentState = FailureState;
        }
        break;
    case ConvertXMLState:
        if (status == 1)
        {
            fsm->currentState = FinalState;
            printf("**Final State**\n");
        }
        else
        {
            fsm->currentState = FailureState;
        }
        break;
    case FinalState:
        printf("\n\n");
        printf("XML Successfully Created.\n");
        break;
    case FailureState:
        printf("\n\n");
        printf("Processing Failed\n");
        break;
    }
}

int main(int argc, char **argv)
{
    MIDINoteFSM fsm;
    // unsigned char buffer[100];
    FILE *midiFile;
    int maxNotes = 3000;
    int i, j;
    struct MidiHeader *midiheader = malloc(sizeof(struct MidiHeader));

    initFSM(&fsm, 1);
    processMidi(&fsm, 1);
    printf("--- test readmidifile ---\n");
    /* ProcessMidiFileState */
    MidiFile *testMidi = malloc(sizeof(struct MidiFile));
    if (testMidi == NULL)
    {
        processMidi(&fsm, -1);
    }
    testMidi = readMidiFile(argv[1]);
    if (testMidi == NULL)
    {
        processMidi(&fsm, -1);
    }

    // MidiNote *midiNotes = createMidiNote(testMidi, maxNotes);
    // printf("midiNotes = %p\n", midiNotes);
    // printf("midiNotes next = %p\n", midiNotes->next);

    processMidi(&fsm, 1);
    /* IntermediateProcessState */
    printf("--- Creating MusicXML file ---\n");
    FILE *xmlFile = fopen("output.xml", "w");
    if (!xmlFile)
    {
        fprintf(stderr, "Failed to create MusicXML file.\n");
        processMidi(&fsm, -1);
    }

    createHeaders(xmlFile); // Start of the XML file
    createPartList(xmlFile, testMidi->header.numberOfTracks);
    int currentPartNumber = 0;

    /* ConvertXMLState */
    processMidi(&fsm, 1);
    for (j = 0; j < testMidi->header.numberOfTracks; j++)
    {
        printf("current for loop interation = %d\n", j);
        MidiNote *trackNotes = createMidiNoteForTrack(testMidi, testMidi->tracks[j], maxNotes);
        printf("trackNotes address = %p\n", trackNotes);
        int counter = 0;
        if (trackNotes != NULL)
        {
            createPart(xmlFile, &j, testMidi->header.timeDivision, trackNotes->keySignature, trackNotes->beats, trackNotes->beatsType, trackNotes->instrument);
            while (trackNotes->next != NULL)
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
    /* FinalState */
    processMidi(&fsm, 1);

    /* Check */
    processMidi(&fsm, 1);

    closeHeaders(xmlFile);
    fclose(midiFile);
}
