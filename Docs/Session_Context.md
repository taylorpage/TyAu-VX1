# VX1 Session Context — Resume Document

This document captures the complete current state of the project so development can resume
in a fresh context window without losing any history.

---

## Project Overview

**TyAu-VX1** — A professional vocal compressor Audio Unit plugin for macOS.
Target sound: JJP-style aggressive, shiny, punchy vocal compression.
Built with: Swift (AU host + UI), C++ (DSP kernel), SwiftUI.

**Xcode project**: `VX1.xcodeproj`
**Scheme**: `VX1` (Debug build works, last verified BUILD SUCCEEDED)

---

## Current Parameter List (14 parameters, addresses 0–13)

| Address | Identifier | Name | Type | Range | Default |
|---------|-----------|------|------|-------|---------|
| 0 | threshold | Threshold | dB | -50…0 | -20 |
| 1 | ratio | Ratio | ratio | 1…30 | 4 |
| 2 | attack | Attack | ms | 0…200 | 10 |
| 3 | release | Release | ms | 5…5000 | 100 |
| 4 | makeupGain | Makeup Gain | dB | -20…+50 | 0 |
| 5 | bypass | Bypass | bool | 0…1 | 0 |
| 6 | mix | Mix | % | 0…100 | 100 |
| 7 | knee | Knee | dB | 0…12 | 3 |
| 8 | detection | Detection | % | 0…100 | 100 |
| 9 | sheen | Sheen | % | 0…100 | 25 |
| 10 | autoMakeup | Auto Makeup | bool | 0…1 | 0 |
| 11 | gainReductionMeter | Gain Reduction | dB | 0…60 | 0 (read-only) |
| 12 | lookAhead | Look-Ahead | indexed | 0…3 | 0 (Off/2ms/5ms/10ms) |
| 13 | inputGain | Input | dB | 0…24 | 0 |

> Note: `drive` was renamed to `sheen` in Sprint 2. `inputGain` (address 13) is fully implemented.

---

## Key Files

```
VX1.xcodeproj
VX1Extension/
  DSP/
    VX1ExtensionDSPKernel.hpp         ← All signal processing
  Parameters/
    VX1ExtensionParameterAddresses.h  ← C enum of parameter addresses
    Parameters.swift                   ← AU parameter tree definition
  Common/Audio Unit/
    VX1ExtensionAudioUnit.swift        ← AU host, meter timer, latency reporting
  UI/
    VX1ExtensionMainView.swift         ← SwiftUI plugin UI
    GainReductionMeter.swift           ← Gain reduction meter component
Docs/
  Development_Roadmap.md
  VX1_User_Guide.md
  VX1_Saturation_Algorithm.md         ← Documents the Sheen algorithm
  Session_Context.md                  ← THIS FILE
```

---

## Signal Flow (Current)

```
Input
  │
  ├─[Sidechain path — detection only]
  │   input × mInputGainLinear → mono sum
  │   → fixed 80 Hz 2-pole Butterworth HPF
  │   → abs(filtered) → envelope follower (attack/release)
  │   → GR calculation (threshold, ratio, soft knee)
  │   → GR Overshoot check (VCA punch: +3 dB if grJump > 3 dB, hold 0.5ms, release 2ms)
  │
  ├─[Audio path]
  │   input × mInputGainLinear → currentInput
  │   → ring buffer delay (look-ahead: 0/2/5/10ms)
  │   → gainReductionTotal applied (GR + mOvershootDb)
  │   → Sheen saturation (4-stage algorithm — see below)
  │   → makeup gain (manual × auto)
  │   → parallel mix (dry/wet blend)
  │
  └─ Output
```

---

## DSP Features Implemented

### Core Compression
- Feed-forward compressor, single-pole envelope follower
- Threshold: -50 to 0 dB
- Ratio: 1:1 to 30:1
- Soft knee: quadratic interpolation (0–12 dB)
- Attack: 0–200ms, Release: 5–5000ms
- Peak/RMS detection blend (Detection parameter)

### Input Gain (address 13) ✅ IMPLEMENTED
- Pre-compression signal gain: 0 to +24 dB, default 0
- Applied to BOTH sidechain detection AND audio path simultaneously
- Forces more GR at any threshold/ratio setting ("slamming" the compressor)
- Linear gain cached: `mInputGainLinear = pow(10, mInputGainDb/20)`
- UI: cyan "INPUT" knob in Row 1 alongside Threshold and Ratio (60px, 12px spacing)

### GR Overshoot / VCA Punch (internal, no parameter) ✅ IMPLEMENTED
- When GR jumps >3 dB in one sample: trigger overshoot
- Over-applies +3 dB extra GR for 0.5ms hold period
- Then exponentially releases extra GR over ~2ms
- Replicates dbx 160 / SSL G-bus VCA physical overshoot behavior
- Meter shows `totalGainReductionDb` (includes overshoot) for accurate visual
- State: `mPrevGainReductionDb`, `mOvershootDb`, `mOvershootReleaseCoeff`, `mOvershootHoldSamples`, `mOvershootHoldCounter`
- Coefficients computed in `initialize()`: sample-rate adaptive

### Look-Ahead (address 12)
- Off / 2ms / 5ms / 10ms fixed steps
- Ring buffer pre-allocated to max 10ms, never reallocated mid-stream
- Detection runs on undelayed input; audio path delayed by ring buffer
- Host latency reported via `VX1ExtensionAudioUnit.latency` override

### Sidechain HPF (always on, no parameter)
- Fixed 80 Hz, 2-pole Butterworth (12 dB/oct)
- Applied to mono sum of input (with input gain applied) before envelope follower
- Prevents kick/bass from pumping vocal compression
- Coefficients: `computeHpfCoefficients()` in DSP kernel
- State: `mHpfX1/X2`, `mHpfY1/Y2`

### Sheen Saturation (address 9, renamed from "Drive")
Four-stage presence-weighted harmonic saturation:
1. **Pre-emphasis**: +5 dB high shelf @ 3.5 kHz before wave shaper
2. **Asymmetric wave shaper**: tanh + DC offset → stronger 2nd harmonic (sheen/sparkle)
3. **Cubic grit layer**: x³ at 6% blend → 3rd harmonic edge
4. **De-emphasis**: matching -5 dB shelf cut → restores tonal balance, harmonics survive
5. **Gain compensation**: `1 / (0.5 + 0.5×blend)` — fixes old under-compensation bug
- State: per-channel `mPreX1/Y1`, `mDeX1/Y1` vectors
- Coefficients: `computePresenceCoefficients()` in DSP kernel
- Full docs: `Docs/VX1_Saturation_Algorithm.md`

### Auto Makeup Gain (address 10)
- Tracks average GR with 100ms smoothing
- Adds 80% of average GR as makeup (intentional under-compensation for punch)
- Multiplicative with manual makeup gain

### Gain Reduction Meter (address 11, read-only)
- Updated at 60Hz via timer in `VX1ExtensionAudioUnit.swift`
- Shows `totalGainReductionDb` (includes GR overshoot) — what you see = what you hear
- Smooth ballistics: fast attack (30%), adaptive release
- UI: `GainReductionMeter.swift`, color-coded green→yellow→orange→red

---

## UI Layout (VX1ExtensionMainView.swift)

Window: 350×580px, dark gradient background.

```
[Title: VX1 COMPRESSOR]
[LED indicator]
[Gain Reduction Meter]
Row 1: [INPUT 60px]     [THRESHOLD 60px]  [RATIO 60px]   (spacing 12px, INPUT label is cyan)
Row 2: [ATTACK 65px]    [RELEASE 65px]
Row 3: [MIX 65px]       [KNEE 65px]       [DETECT 65px]
Row 4: [MAKEUP 65px]    [SHEEN 65px]
       └[AUTO toggle]
Row 5: [LOOK-AHEAD segmented: Off / 2ms / 5ms / 10ms]
[BYPASS button]
[TaylorAudio logo]
```

---

## Sprint Status

### Sprint 1 ✅ COMPLETE
1. ✅ Auto Makeup Gain
2. ✅ Gain Reduction Meter
3. ✅ DSP Stability (no pops)
4. ✅ Look-Ahead with ring buffer + host latency

### Sprint 2 (In Progress)
1. ✅ Sidechain HPF — fixed 80 Hz, always on
2. ✅ Sheen Saturation — 4-stage JJP presence algorithm (renamed from Drive)
3. ✅ Input Gain — pre-compression drive, 0–24 dB (address 13)
4. ✅ GR Overshoot — VCA punch (internal, no user parameter)
5. ⬜ Input/Output Metering
6. ⬜ Presets System

### Sprint 3 (Planned)
1. De-esser
2. RMS Window Control
3. Mid/Side Processing

---

## Architecture Notes

- All DSP is in `VX1ExtensionDSPKernel.hpp` (C++ class, render-thread safe)
- All parameters use `VX1ExtensionParameterAddress` enum (C header shared with Swift via bridging)
- UI uses SwiftUI with `ParameterKnob`, `GainReductionMeter`, `BypassButton` components
- 60Hz meter timer in `VX1ExtensionAudioUnit.swift`
- Host latency reported via `latency` override in same file
- Ring buffer always allocated at max capacity (never mid-stream realloc)
- All filter coefficients computed in `initialize()` — correct across 44.1/48/96 kHz
- **14 parameters** (addresses 0–13); gainReductionMeter (address 11) is read-only

---

## Key DSP Code Snippets

### Input Gain Application (process loop)
```cpp
// Sidechain: apply input gain before HPF
monoSC += inputBuffers[channel][frameIndex] * mInputGainLinear;

// Audio path: apply input gain
float currentInput = inputBuffers[channel][frameIndex] * mInputGainLinear;
```

### GR Overshoot (process loop, after GR calculation)
```cpp
float grJump = gainReductionDb - mPrevGainReductionDb;
if (grJump > 3.0f) {
    mOvershootDb = 3.0f;
    mOvershootHoldCounter = mOvershootHoldSamples;
}
mPrevGainReductionDb = gainReductionDb;

if (mOvershootHoldCounter > 0) {
    mOvershootHoldCounter--;
} else {
    mOvershootDb *= mOvershootReleaseCoeff;
}

float totalGainReductionDb = gainReductionDb + mOvershootDb;
float gainReductionTotal = std::pow(10.0f, -totalGainReductionDb / 20.0f);
peakGainReductionDb = std::max(peakGainReductionDb, totalGainReductionDb);
```

### initialize() additions for super-compression
```cpp
mInputGainLinear       = std::pow(10.0f, mInputGainDb / 20.0f);
mOvershootReleaseCoeff = std::exp(-1.0f / (0.002f * (float)mSampleRate));
mOvershootHoldSamples  = static_cast<int>(0.0005f * mSampleRate);
```

---

## NEXT TASKS (Sprint 2 remaining)

### Input/Output Metering
- Add input level meter and output level meter
- Show peak and/or RMS
- Visual feedback for gain staging workflow

### Presets System
- Factory presets browser
- User preset save/load
- Suggested presets: JJP Style, Transparent, Thick Parallel, Slam, Ballad
