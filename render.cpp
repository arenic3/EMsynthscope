/*
Electromagnetic groove machine for Bela.
Analog 0 sets absolute acid resonance; analog 1 optionally adds cutoff modulation.
*/

#include <Bela.h>
#include <libraries/Scope/Scope.h>
#include "Filter.h"
#include "Sequencer.h"
#include "Voices.h"

namespace {
constexpr float kBpm = 124.0f;
constexpr float kSwing = 0.0f; // 0 = straight; 0.15 = 57.5/42.5 long/short.
constexpr bool kSquareWave = false;
constexpr bool kUseSecondInputForCutoff = false;
constexpr float kBaseCutoff = 250.0f;
constexpr float kEnvelopeAmount = 2200.0f;
constexpr float kMasterGain = 1.2f;
constexpr float kGateFraction = 0.65f;

// Bela analog values are nominally 0..1. Calibrate for your pickup and gain.
constexpr float kEmCenter = 0.5f;
constexpr float kEmScale = 2.0f;
constexpr float kEmFloor = 0.02f;
constexpr float kEmCeiling = 0.6f;
constexpr float kEmSmoothingHz = 20.0f;
constexpr float kMinResonance = 0.7f;
constexpr float kMaxResonance = 8.0f;
static_assert(kBpm > 0.0f, "BPM must be positive");
static_assert(kEmCeiling > kEmFloor, "EM calibration range must be positive");

Scope gScope;
Sequencer gSequencer;
DrumVoices gDrums;
AcidVoice gAcid;
LowPassFilter gEmA(44100.0f);
LowPassFilter gEmB(44100.0f);
float gLevelA = 0.0f;
float gLevelB = 0.0f;
double gGateRemaining = 0.0;
bool gSlideToNext = false;

float readMagnitude(BelaContext* context, unsigned int frame, unsigned int channel) {
	float raw = analogRead(context, frame, channel);
	if(!std::isfinite(raw)) return 0.0f;
	return std::min(1.0f, std::fabs((raw - kEmCenter) * kEmScale));
}

float normalizedLevel(float magnitude) {
	return std::max(0.0f, std::min(1.0f,
		(magnitude - kEmFloor) / (kEmCeiling - kEmFloor)));
}
}

bool setup(BelaContext* context, void*) {
	if(context->audioSampleRate <= 0.0f || context->audioFrames == 0 ||
		context->audioOutChannels == 0 || context->analogFrames == 0 ||
		context->analogInChannels == 0) {
		rt_printf("Need audio output and at least one enabled analog input\n");
		return false;
	}
	gSequencer.setup(context->audioSampleRate, kBpm, kSwing);
	gDrums.setup(context->audioSampleRate);
	gAcid.setup(context->audioSampleRate);
	// Reuse the original two-stage low-pass as a rectified EM envelope follower.
	gEmA = LowPassFilter(context->audioSampleRate);
	gEmB = LowPassFilter(context->audioSampleRate);
	gEmA.setCutoff(kEmSmoothingHz);
	gEmB.setCutoff(kEmSmoothingHz);
	gEmA.setThreshold(1.0f);
	gEmB.setThreshold(1.0f);
	gLevelA = gLevelB = 0.0f;
	gGateRemaining = 0.0;
	gSlideToNext = false;
	gScope.setup(3, context->audioSampleRate);
	return true;
}

void render(BelaContext* context, void*) {
	for(unsigned int n = 0; n < context->audioFrames; ++n) {
		// Use actual buffer sizes rather than assuming analog always runs at half rate.
		unsigned int analogFrame = static_cast<unsigned int>(
			static_cast<uint64_t>(n) * context->analogFrames / context->audioFrames);
		gLevelA = gEmA.process(readMagnitude(context, analogFrame, 0));
		gLevelB = context->analogInChannels > 1
			? gEmB.process(readMagnitude(context, analogFrame, 1)) : 0.0f;
		float resonance = kMinResonance
			+ normalizedLevel(gLevelA) * (kMaxResonance - kMinResonance);
		float cutoff = kBaseCutoff;
		if(kUseSecondInputForCutoff)
			cutoff += 1800.0f * normalizedLevel(gLevelB);

		if(gSequencer.tick()) {
			const GrooveStep& step = kPattern[gSequencer.step()];
			if(step.kick) gDrums.kick();
			if(step.snare) gDrums.snare();
			if(step.hat) gDrums.hat();
			if(step.note >= 0) {
				gAcid.noteOn(step.note, step.accent, gSlideToNext);
				gSlideToNext = step.slide &&
					kPattern[(gSequencer.step() + 1) % 16].note >= 0;
				gGateRemaining = gSlideToNext ? 0.0
					: gSequencer.duration() * kGateFraction;
			} else {
				gAcid.noteOff();
				gSlideToNext = false;
				gGateRemaining = 0.0;
			}
		}
		if(!gSlideToNext && gGateRemaining <= 0.0)
			gAcid.noteOff();
		float mix = gDrums.process()
			+ gAcid.process(cutoff, resonance, kEnvelopeAmount, kSquareWave);
		float out = 0.95f * std::tanh(kMasterGain * mix);
		for(unsigned int channel = 0; channel < context->audioOutChannels; ++channel)
			audioWrite(context, n, channel, out);
		gScope.log(out, gLevelA, gLevelB);
		gGateRemaining -= 1.0;
	}
}

void cleanup(BelaContext*, void*) {
	// All DSP state is owned statically; no MIDI device or heap resources to release.
}
