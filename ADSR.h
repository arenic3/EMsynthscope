#ifndef ADSR_H
#define ADSR_H
#include <cmath>

class ADSR {
	public:
		ADSR();
		ADSR(float sampleRate);
		void setSampleRate(float rate);
		void noteOn();
		void noteOff();
		float getNextValue();
		
		void setAttack(float attackTime);
		void setRelease(float releaseTime);
		
		
	private:
		float sampleRate;
		float attackTime_;
		float releaseTime_;
		enum State {OFF, ATTACK, SUSTAIN, RELEASE} currentState;
		float level;	
		float attackIncrement;
		float releaseIncrement;
};

#endif