#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <algorithm>

struct GrooveStep {
	bool kick;
	bool snare;
	bool hat;
	int note; // MIDI pitch; -1 is a rest.
	bool accent;
	bool slide; // Tie/glide into the following note without retriggering.
};

constexpr GrooveStep kPattern[16] = {
	{true,  false, true,  36, true,  false},
	{false, false, true,  -1, false, false},
	{false, false, true,  36, false, true },
	{false, false, true,  43, false, false},
	{false, true,  true,  36, true,  false},
	{false, false, true,  -1, false, false},
	{true,  false, true,  48, false, true },
	{false, false, true,  43, true,  false},
	{true,  false, true,  36, true,  false},
	{false, false, true,  -1, false, false},
	{false, false, true,  39, false, false},
	{true,  false, true,  43, false, false},
	{false, true,  true,  36, true,  false},
	{false, false, true,  43, false, true },
	{false, false, true,  48, true,  true },
	{false, false, true,  43, false, true }
};

class Sequencer {
public:
	void setup(float sampleRate, float bpm, float swing) {
		stepSamples_ = sampleRate * 60.0 / (std::max(1.0f, bpm) * 4.0);
		swing_ = std::max(0.0f, std::min(0.49f, swing));
		remaining_ = 0.0;
		step_ = 15;
	}

	// Call exactly once per audio sample, retaining fractional timing across blocks.
	bool tick() {
		bool triggered = remaining_ <= 0.0;
		if(triggered) {
			step_ = (step_ + 1) % 16;
			remaining_ += duration();
		}
		remaining_ -= 1.0;
		return triggered;
	}

	unsigned int step() const { return step_; }
	double duration() const {
		return stepSamples_ * (1.0 + (step_ % 2 ? -swing_ : swing_));
	}

private:
	double stepSamples_ = 1.0;
	double remaining_ = 0.0;
	float swing_ = 0.0f;
	unsigned int step_ = 15;
};

#endif
