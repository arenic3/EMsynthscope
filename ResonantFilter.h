#ifndef RESONANT_FILTER_H
#define RESONANT_FILTER_H

#include <algorithm>
#include <cmath>

class ResonantFilter {
public:
	void setup(float sampleRate) {
		sampleRate_ = sampleRate;
		state1_ = state2_ = 0.0f;
	}

	float process(float input, float cutoff, float resonance) {
		cutoff = std::max(20.0f, std::min(sampleRate_ * 0.2f, cutoff));
		// Bounded Q and a topology-preserving SVF keep feedback stable.
		float q = std::max(0.7f, std::min(8.0f, resonance));
		float g = std::tan(3.14159265359f * cutoff / sampleRate_);
		float damping = 1.0f / q;
		float a = 1.0f / (1.0f + g * (g + damping));
		float v1 = a * (state1_ + g * (input - state2_));
		float v2 = state2_ + g * v1;
		state1_ = 2.0f * v1 - state1_;
		state2_ = 2.0f * v2 - state2_;
		return v2;
	}

private:
	float sampleRate_ = 44100.0f;
	float state1_ = 0.0f;
	float state2_ = 0.0f;
};

#endif
