#ifndef VOICES_H
#define VOICES_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "ResonantFilter.h"

inline float decayCoefficient(float seconds, float sampleRate) {
	return std::exp(-1.0f / (seconds * sampleRate));
}

class DrumVoices {
public:
	void setup(float sampleRate) {
		sampleRate_ = sampleRate;
		kickDecay_ = decayCoefficient(0.16f, sampleRate);
		pitchDecay_ = decayCoefficient(0.025f, sampleRate);
		snareDecay_ = decayCoefficient(0.09f, sampleRate);
		hatDecay_ = decayCoefficient(0.025f, sampleRate);
		noiseLowpassAlpha_ = 1.0f - decayCoefficient(1.0f / (6.2831853f * 7000.0f), sampleRate);
		kickEnv_ = pitchEnv_ = snareEnv_ = hatEnv_ = 0.0f;
		kickPhase_ = snarePhase_ = noiseLowpass_ = 0.0f;
		noiseState_ = 1;
	}

	void kick() { kickEnv_ = pitchEnv_ = 1.0f; kickPhase_ = 0.0f; }
	void snare() { snareEnv_ = 1.0f; snarePhase_ = 0.0f; }
	void hat() { hatEnv_ = 1.0f; }

	float process() {
		// Local deterministic PRNG: no allocation, locking or global rand() state.
		noiseState_ ^= noiseState_ << 13;
		noiseState_ ^= noiseState_ >> 17;
		noiseState_ ^= noiseState_ << 5;
		float noise = (noiseState_ >> 8) * (2.0f / 16777215.0f) - 1.0f;
		noiseLowpass_ += noiseLowpassAlpha_ * (noise - noiseLowpass_);
		float out = 0.8f * kickEnv_ * std::sin(6.2831853f * kickPhase_);
		out += 0.35f * snareEnv_ * (0.8f * noise + 0.2f * std::sin(6.2831853f * snarePhase_));
		out += 0.22f * hatEnv_ * (noise - noiseLowpass_);
		kickPhase_ += (48.0f + 140.0f * pitchEnv_) / sampleRate_;
		snarePhase_ += 180.0f / sampleRate_;
		kickPhase_ -= std::floor(kickPhase_);
		snarePhase_ -= std::floor(snarePhase_);
		kickEnv_ *= kickDecay_;
		pitchEnv_ *= pitchDecay_;
		snareEnv_ *= snareDecay_;
		hatEnv_ *= hatDecay_;
		// Avoid denormals during rests.
		if(kickEnv_ < 1e-8f) kickEnv_ = 0.0f;
		if(pitchEnv_ < 1e-8f) pitchEnv_ = 0.0f;
		if(snareEnv_ < 1e-8f) snareEnv_ = 0.0f;
		if(hatEnv_ < 1e-8f) hatEnv_ = 0.0f;
		return out;
	}

private:
	float sampleRate_ = 44100.0f;
	float kickDecay_, pitchDecay_, snareDecay_, hatDecay_, noiseLowpassAlpha_;
	float kickEnv_ = 0.0f, pitchEnv_ = 0.0f, snareEnv_ = 0.0f, hatEnv_ = 0.0f;
	float kickPhase_ = 0.0f, snarePhase_ = 0.0f, noiseLowpass_ = 0.0f;
	uint32_t noiseState_ = 1;
};

class AcidVoice {
public:
	void setup(float sampleRate) {
		sampleRate_ = sampleRate;
		filter_.setup(sampleRate);
		envDecay_ = decayCoefficient(0.18f, sampleRate);
		glide_ = 1.0f - decayCoefficient(0.06f, sampleRate);
		attack_ = 1.0f - decayCoefficient(0.003f, sampleRate);
		release_ = decayCoefficient(0.04f, sampleRate);
		phase_ = amp_ = env_ = 0.0f;
		gate_ = false;
		frequency_ = targetFrequency_ = 65.406f;
	}

	void noteOn(int note, bool accent, bool slideFromPrevious) {
		note = std::max(0, std::min(127, note));
		targetFrequency_ = std::min(sampleRate_ * 0.2f,
			440.0f * std::pow(2.0f, (note - 69) / 12.0f));
		if(!slideFromPrevious || !gate_) {
			frequency_ = targetFrequency_;
			env_ = 1.0f;
		}
		accent_ = accent;
		gate_ = true;
	}

	void noteOff() { gate_ = false; }

	float process(float cutoff, float resonance, float envAmount, bool square) {
		frequency_ += glide_ * (targetFrequency_ - frequency_);
		float increment = frequency_ / sampleRate_;
		// PolyBLEP edges reduce aliasing for both selectable waveforms.
		float wave = 2.0f * phase_ - 1.0f - polyBlep(phase_, increment);
		if(square) {
			float shifted = phase_ + 0.5f;
			if(shifted >= 1.0f) shifted -= 1.0f;
			wave = (phase_ < 0.5f ? 1.0f : -1.0f)
				+ polyBlep(phase_, increment) - polyBlep(shifted, increment);
		}
		phase_ += increment;
		if(phase_ >= 1.0f) phase_ -= 1.0f;
		float targetAmp = accent_ ? 0.8f : 0.55f;
		if(gate_) amp_ += attack_ * (targetAmp - amp_);
		else amp_ *= release_;
		env_ *= envDecay_;
		if(amp_ < 1e-8f) amp_ = 0.0f;
		if(env_ < 1e-8f) env_ = 0.0f;
		float filtered = filter_.process(wave * amp_,
			cutoff + envAmount * env_ * (accent_ ? 1.6f : 1.0f), resonance);
		return 0.35f * filtered;
	}

private:
	static float polyBlep(float phase, float increment) {
		if(phase < increment) {
			float t = phase / increment;
			return t + t - t * t - 1.0f;
		}
		if(phase > 1.0f - increment) {
			float t = (phase - 1.0f) / increment;
			return t * t + t + t + 1.0f;
		}
		return 0.0f;
	}

	ResonantFilter filter_;
	float sampleRate_ = 44100.0f;
	float frequency_ = 65.406f, targetFrequency_ = 65.406f;
	float phase_ = 0.0f, amp_ = 0.0f, env_ = 0.0f;
	float envDecay_, glide_, attack_, release_;
	bool gate_ = false, accent_ = false;
};

#endif
