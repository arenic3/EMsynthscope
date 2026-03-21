/*
Electro magnetic synth, using two ungrounded analong wire inputs with a bela board,
analog input 0 controls freq mod and analog 1 controls lowpass filter cutoff.
Fun to play with if you are better at the keyboard than me.

-Nicolas Arellano
*/

#include <Bela.h>
#include <vector> 
#include <libraries/Midi/Midi.h>
#include <libraries/Scope/Scope.h>
#include <stdlib.h>
#include "ADSR.h"
#include "Filter.h"

Midi gMidi;
ADSR adsr;
Filter* lowpass = nullptr;
Scope gScope;

const char* gMidiPort0 = "hw:0,0";

float freq = 220.0f;
float amp = 1;
float phase = 0;

float att = 1;
float rel = 1;
float cutoff = 300;
float thresh = 0.8;

float gAntennaA = 0.0f;
float gAntennaB = 0.0f;

const int maxNotes = 16;
int activeNotes[maxNotes];
int activeNoteCount = 0;

void midiListener(MidiChannelMessage message, void *arg);


bool setup(BelaContext *context, void *userData) {
	
	for(int i = 0; i < maxNotes; i++) activeNotes[i] = 0;
	
	if(gMidi.readFrom(gMidiPort0) < 0) {
		rt_printf("unable to read from midi port");
		return false;
	}
	
	gMidi.writeTo(gMidiPort0);
	gMidi.enableParser(true);
	gMidi.setParserCallback(midiListener, (void*)gMidiPort0);
	
	adsr.setSampleRate(context->audioSampleRate);
	
	lowpass = new LowPassFilter(context->audioSampleRate);
	lowpass->setCutoff(cutoff);
	lowpass->setThreshold(thresh);
	
	if(context->analogInChannels < 2) {
		rt_printf("Error: need at least 2 analog input channels for antennas\n");
		return false;
	}
	
	gScope.setup(3, context->audioSampleRate);
	
	return true;
}

float mapF(int noteNumber) {
	return powf(2.0, (noteNumber - 69)/12.0) * 440.0;
}

void keyPressed(int noteNumber, int velocity) {
	if (activeNoteCount < maxNotes) {
		activeNotes[activeNoteCount] = noteNumber;
		activeNoteCount++;
		
		freq = mapF(noteNumber);
		
		float db = map(velocity, 1, 127, -40, 0);
		amp = powf(10.0, db / 20.0);
		
		if (activeNoteCount == 1) {
			adsr.noteOn();
		}
		
		rt_printf("note on: frequency: %f, amplitude: %f\n", freq, amp);
	}
}

void keyReleased(int noteNumber) {
	
	bool activeNoteChanged = false;
	
	for (int i = activeNoteCount - 1; i >= 0; i--) {
		if (activeNotes[i] == noteNumber) {
			if (i == activeNoteCount - 1) {
				activeNoteChanged = true;
			}
			
			for (int j = i; j < activeNoteCount - 1; j++) {
				activeNotes[j] = activeNotes[j + 1];
			}
			activeNoteCount--;
		}
	}
	
	if (activeNoteCount == 0) {
		adsr.noteOff();
	} else if (activeNoteChanged) {
		int mostRecentNote = activeNotes[activeNoteCount - 1];
		
		freq = mapF(mostRecentNote);
	}
}

void render(BelaContext *context, void *userData) {
	
	float envA = att;
	float envR = rel;
	
	float fCut = cutoff;
	
	adsr.setAttack(envA);
	adsr.setRelease(envR);
	
	lowpass->setCutoff(fCut);
	lowpass->setThreshold(thresh);
	
    for (unsigned int n = 0; n < context->audioFrames; n++) {
    	
    	gAntennaA = (analogRead(context, n/2, 0) - 0.5f) * 2.0f;
    	gAntennaB = (analogRead(context, n/2, 1) - 0.5f) * 2.0f;
    	float antCutoff = map(fabsf(gAntennaA), 0.0f, 1.0f, cutoff * 0.5f, cutoff * 2.0f);
    	lowpass->setCutoff(antCutoff);
    	
    	float out = 0.0f;
    	float oAmp = amp * adsr.getNextValue();
    	
    	if (activeNoteCount > 0) {
    		float fMod = freq * (1.0f + gAntennaB * 0.02f);
    		
    		phase += fMod/context->audioSampleRate;
    		if (phase >= 1.0f) phase -= 1.0f;
    		float saw = 2.0f * ((phase) - std::floor(phase)) - 1.0f;
    		
    		out = saw * oAmp;
    	}
    	
    	float filtered = lowpass->process(out);
    	
    	for (unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
    		audioWrite(context, n, channel, filtered);
    	}
    	
    	gScope.log(gAntennaA, gAntennaB, filtered);
    }
}

void midiListener (MidiChannelMessage message, void *arg) {
	
	if (message.getType() == kmmNoteOn) {
			int noteNumber = message.getDataByte(0);
			int velocity = message.getDataByte(1);
			
			if (velocity == 0) {
				keyReleased(noteNumber);
				
			} else {
				keyPressed(noteNumber, velocity);
			}
			
		} else if (message.getType() == kmmNoteOff) {
			int noteNumber = message.getDataByte(0);
			
			keyReleased(noteNumber);
		}
}

void cleanup(BelaContext *context, void *userData) {
	gMidi.enableParser(false);
	
	if(lowpass != nullptr){
		delete lowpass;
		lowpass = nullptr;
	}
}
