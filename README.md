# MIDINote
## Overview

MIDINote is a lightweight system that converts (Musical Instrument Digital Interface) MIDI Files into Musical Score Sheets (MusicXML format) for both musicians and producers.
MIDI music can be generated or created quickly with today’s technology. 
However, musicians still rely on traditional musical notation to perform and play their instrument. As such, there exists a gap where MIDI music is not presented in a form that can be understood and deciphered by musicians as it is not in a human-readable form such as text.

## Prerequisite
- GCC Compiler
- Make

## Building the Project
1. Clone the repository to your local machine.
2. Navigate to the project's src directory.
3. Run `make` to build the project. This will compile the source code and generate an executable.
4. Run the programme with a MIDI file to generate a MusicXML file.

### Cloning the repository to your local machine.
Simply type in the following in your terminal
```shell
git clone https://github.com/bojx96/MIDINote_Submission.git
```

### Run the programme with a MIDI file to generate a MusicXML file.
```shell
./main <input_midi_file>
```
you may take an MIDI file from our testfiles folder, for example
```shell
./main ../testfiles/omg-h.mid
```
The generated MusicXML file can be found as the 'output.xml' file.




