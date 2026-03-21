#include "ADSR.h"

ADSR::ADSR()
	: currentState(OFF), level(0.0f), attackIncrement(0), releaseIncrement(0) 
	{
		attackTime_ = 0.001;
		releaseTime_= 0.001;
	};
	
void ADSR::setSampleRate(float rate) {
	sampleRate = rate;
}	

void ADSR::setAttack(float attackTime) {
	if (attackTime >= 0) {
		attackTime_ = attackTime;
	} else {
		attackTime_ = 0;
	}
}
	
void ADSR::setRelease(float releaseTime) {
	if (releaseTime >= 0) {
		releaseTime_ = releaseTime;
	} else {
		releaseTime_ = 0;
	}
}

void ADSR::noteOn() {
	currentState = ATTACK;
}

void ADSR::noteOff() {
	currentState = RELEASE;
}

float ADSR::getNextValue() {
	attackIncrement = 1.0f / (sampleRate * attackTime_);
	releaseIncrement = 1.0f / (sampleRate * releaseTime_);
	
	switch(currentState) 
	{
		case ATTACK:
		level+=attackIncrement;
		if(level>1.0f){
			level = 1.0f;
			currentState = SUSTAIN;
		}
		break;
		
		case SUSTAIN:
		break;
		
		case RELEASE:
		level-=releaseIncrement;
		if(level<0.0f) {
			level=0.0f;
			currentState = OFF;
		}
		break;
		
		default:
		level=0.0f;
	}
	
	return level;
}