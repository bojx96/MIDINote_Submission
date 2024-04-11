#include <stdio.h>
#include <string.h>
#include <main.h>

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
    else
    {
        clefSign = 'G';
        clefLine = 2;
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
