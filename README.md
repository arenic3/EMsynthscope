# Bela Electromagnetic Groove Machine

A standalone, repeating 16-step drum/acid machine controlled by an EM pickup on
Bela analog input 0. No MIDI controller, samples, buttons, or programming UI are
required. Playback starts at step 0 when the audio callback starts.

## Pattern and tempo

Edit `kPattern` in `Sequencer.h`. Each of its 16 entries is:

```cpp
// kick, snare, closed hat, MIDI pitch, accent, slide to next step
{true, false, true, 36, true, false}
```

Drum flags trigger synthesized voices: pitch-swept sine kick, noise/tone snare,
and short high-passed-noise hat. Pitch `-1` is an acid rest; `36` is C2.
Accent increases acid amplitude and filter-envelope depth. Slide ties the current
note into the **following** note, gliding pitch without retriggering the envelope;
a following rest always releases it. Slides also work across the loop boundary.
Ordinary notes use a 65% gate with a short release.

In `render.cpp`, set `kBpm` (default 124) and `kSwing` (default 0, straight).
Each step is a sixteenth note. Swing alternates durations by `1 + swing` and
`1 - swing`, preserving average BPM; `0.15` gives a 57.5/42.5 pair.
The clock runs per audio sample, not per callback, carrying fractional samples
across buffers to avoid cumulative rounding drift (events quantize within one
sample). Supported swing is 0 through 0.49.

The acid voice uses a band-limited saw (or square with `kSquareWave = true`),
a resonant low-pass, a decaying cutoff envelope, accent, and 60 ms pitch glide.
This is a 303-style synthesis voice, not a circuit-exact emulation.
`kBaseCutoff`, `kEnvelopeAmount`, `kGateFraction`, and `kMasterGain` are also
editable constants. The master is soft-clipped with `tanh` to below ±0.95.
The original MIDI/melodic path is removed; the unused ADSR utility remains
available, and the original two-stage `LowPassFilter` is reused for EM smoothing.

## EM control and calibration

Analog 0's nominal 0..1 reading is centered and rectified:
`magnitude = abs((raw - kEmCenter) * kEmScale)`, clamped to 0..1.
The existing two-stage low-pass smooths that magnitude at `kEmSmoothingHz`
(20 Hz). Rectifying **before** smoothing prevents alternating pickup signals
from cancelling. This level sets **absolute** resonance, not a relative offset
or an accumulated change:

```text
level = clamp((smoothedMagnitude - kEmFloor) / (kEmCeiling - kEmFloor), 0, 1)
Q = kMinResonance + level * (kMaxResonance - kMinResonance)
```

Defaults are center 0.5, scale 2, floor 0.02, ceiling 0.6, and Q 0.7..8.
The filter independently bounds Q to 0.7..8 and cutoff to 20 Hz..20% of the
audio sample rate for stable feedback. Start with low listening volume: strong
resonance can still produce loud narrow-band peaks despite output limiting.

Analog 1 is optional. Its magnitude is smoothed the same way and shown on the
scope. Set `kUseSecondInputForCutoff = true` to add 0..1800 Hz to the base cutoff
using the same calibration range; by default it does not affect the sound.
One enabled analog input is enough; when channel 1 is absent its level is zero.
Analog frames are selected using the actual analog/audio buffer-size ratio,
rather than assuming a fixed half-rate analog configuration.

When hardware is connected:

1. Use a safely conditioned, Bela-compatible pickup input. Never connect mains
   or other over-voltage signals directly; respect the board's analog limits.
2. Scope channels are **master output, smoothed EM 0 magnitude, smoothed EM 1
   magnitude**. Observe quiet and active pickup levels first.
3. Set `kEmCenter` to the pickup's idle DC bias and adjust `kEmScale` if needed.
   Set `kEmFloor` just above idle magnitude and `kEmCeiling` to a useful strong
   pickup level (strictly greater than the floor). Changes affect both inputs.
4. Adjust smoothing for responsiveness versus jitter; keep Q within the filter's
   safe range. Hardware listening, calibration, and CPU-load testing are still
   required.

## Running on Bela

Copy this project's `.cpp`/`.h` files to a Bela IDE project and Build & Run,
with audio output and at least one analog input enabled. The project uses Bela's
`setup`/`render`/`cleanup` entry points and Scope library; the render callback has
no heap allocation or MIDI-device dependency. No standalone build/test runner
or Bela SDK is bundled in this repository.
