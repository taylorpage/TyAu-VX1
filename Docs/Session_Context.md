# VX1 Session Context — Resume Document

This document captures the complete current state of the project so development can resume
in a fresh context window without losing any history.

---

## Project Overview

**TyAu-VX1** — A professional vocal compressor Audio Unit plugin for macOS.
Target sound: Character compressor for vocals — aggressive, present, punchy.
Built with: Swift (AU host + UI), C++ (DSP kernel), SwiftUI.

**Xcode project**: `VX1.xcodeproj`
**Scheme**: `VX1` (Debug build, last verified BUILD SUCCEEDED)

---

## Current Parameter List (11 parameters, addresses 0–14)

| Address | Identifier | Name | Type | Range | Default |
|---------|-----------|------|------|-------|---------|
| 0 | threshold | Threshold | dB | -50…0 | -20 |
| 1 | ratio | Ratio | ratio | 1…30 | 4 |
| 2 | attack | Attack | ms | 0…200 | 10 |
| 3 | release | Release | ms | 5…5000 | 100 |
| 4 | makeupGain | Makeup Gain | dB | -20…+50 | 0 |
| 5 | bypass | Bypass | bool | 0…1 | 0 |
| 6 | mix | Mix | % | 0…100 | 100 |
| 7 | *(reserved)* | — | — | — | — |
| 8 | grip | Grip | % | 0…100 | 0 |
| 9 | bite | Bite | % | 0…100 | 25 |
| 11 | gainReductionMeter | Gain Reduction | dB | 0…60 | 0 (read-only) |
| 14 | gateThreshold | Gate | dB | -80…-20 | -80 (off) |

> Addresses 7, 10, 12, 13 reserved/removed. inputGain (13) removed — was redundant with threshold on a character compressor. detection/sheen renamed to grip/bite.

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
    VUMeter.swift                      ← Gain reduction meter component
Docs/
  Development_Roadmap.md
  VX1_User_Guide.md
  Session_Context.md                  ← THIS FILE
```

---

## Signal Flow (Current)

```
Input
  │
  ├─[Noise Gate]
  │   raw input → peak envelope follower (0.5ms attack, 50ms hold, 100ms release)
  │   → compare to gateThreshold → mGateGain scalar (0=closed, 1=open)
  │   → applied to both sidechain and audio paths
  │
  ├─[Sidechain path — detection only]
  │   input × mGateGain → mono sum
  │   → fixed 80 Hz 2-pole Butterworth HPF
  │   → peak (instantaneous abs) + RMS (175ms IIR: mRmsState = mRmsCoeff*mRmsState + (1-mRmsCoeff)*sample²)
  │   → blended by Grip: detectionLevel = rms*(1-grip) + peak*grip
  │   → envelope follower: attack = mAttackCoeff*(1-grip) + mInstantCoeff*grip, release = mReleaseCoeff
  │   → GR calculation (threshold, ratio, hard knee — fixed)
  │   → GR Overshoot check (VCA punch: +3 dB if grJump > 3 dB, hold 0.5ms, release 2ms)
  │
  ├─[Audio path]
  │   input × mGateGain → audioInput
  │   → gainReductionTotal applied (GR + mOvershootDb)
  │   → Bite saturation (4-stage algorithm — see below)
  │   → makeup gain
  │   → parallel mix (dry/wet blend)
  │
  └─ Output
```

---

## DSP Features

### Grip (address 8) — Dual-Layer Peak/RMS Detection
- **0% (full left)**: RMS mode — 175ms IIR window + user's attack knob time. Smooth, musical, vocal-friendly.
- **100% (full right)**: Peak mode — instantaneous abs + 2ms fixed attack. Aggressive, grabs every transient.
- Two things change simultaneously: the detected level signal AND the envelope attack coefficient.
- RMS coefficient: `exp(-1 / (0.175 * sr))` — ~175ms window, averages across full syllables
- Instant coefficient: `exp(-1 / (0.002 * sr))` — ~2ms, fast but distortion-safe on vocals

### Bite (address 9) — Presence-Biased Harmonic Saturation
Four-stage algorithm, runs post-compression:
1. **Pre-emphasis**: +5 dB high shelf @ 3.5 kHz before wave shaper → biases harmonics toward presence/air band
2. **Asymmetric wave shaper**: `tanh` with DC offset (0–0.18 scaling with blend) → 2nd harmonic generation (shimmer/sparkle)
   - Drive: `1.0 + blend * 4.0` → 1x at 0%, 5x at 100%
3. **Cubic grit**: `x³ * 0.06 * blend * (1 - blend*0.5)` → 3rd harmonic, self-limits at high drive to prevent intermodulation
4. **De-emphasis**: matching -5 dB shelf after wave shaper → restores tonal balance; generated harmonics survive
5. **Gain compensation**: `1 / tanh(drive * 1.3)` — exact normalization, keeps wet path at unity regardless of drive

State: per-channel `mPreX1/Y1`, `mDeX1/Y1` vectors. Coefficients in `computePresenceCoefficients()`.

### GR Overshoot / VCA Punch (internal, no parameter)
- Trigger: GR jumps >3 dB in one sample
- Over-applies +3 dB extra GR for 0.5ms hold, then exponential release over ~2ms
- Replicates dbx 160 / SSL G-bus VCA physical overshoot behavior
- Meter shows `totalGainReductionDb` (includes overshoot)
- State: `mPrevGainReductionDb`, `mOvershootDb`, `mOvershootReleaseCoeff`, `mOvershootHoldSamples`, `mOvershootHoldCounter`

### Noise Gate (address 14)
- Threshold: -80 to -20 dB, default -80 dB (= gate off)
- Fixed timing: 0.5ms attack, 50ms hold, 100ms release
- Peak envelope follower on mono sum of raw input
- `mGateGain` scalar (0–1) applied to both sidechain and audio paths
- State: `mGateEnvelope`, `mGateGain`, `mGateAttackCoeff`, `mGateReleaseCoeff`, `mGateHoldSamples`, `mGateHoldCounter`, `mGateOpen`

### Sidechain HPF (always on, no parameter)
- Fixed 80 Hz, 2-pole Butterworth (12 dB/oct)
- Applied to mono sum before envelope follower
- Prevents low-frequency content from pumping vocal compression
- State: `mHpfX1/X2`, `mHpfY1/Y2`

### Gain Reduction Meter (address 11, read-only)
- Updated at 60Hz via timer in `VX1ExtensionAudioUnit.swift`
- Shows `totalGainReductionDb` (includes GR overshoot)
- Smooth ballistics: fast attack (30% coefficient), adaptive release

---

## UI Layout (VX1ExtensionMainView.swift)

Window: 350×580px, dark gradient background.

```
[Title: VX1 COMPRESSOR]
[LED indicator]
[Gain Reduction Meter]
Row 1: [GATE 55px]  [THRESHOLD 55px]  [RATIO 55px]  (spacing 10px, GATE label orange)
Row 2: [ATTACK 65px]    [RELEASE 65px]
Row 3: [MIX 65px]       [GRIP 65px]
Row 4: [MAKEUP 65px]    [BITE 65px]
[BYPASS button]
[TaylorAudio logo]
```

---

## Sprint Status

### Sprint 1 ✅ COMPLETE
1. ✅ Gain Reduction Meter
2. ✅ DSP Stability (no pops)

### Sprint 2 (In Progress)
1. ✅ Sidechain HPF — fixed 80 Hz, always on
2. ✅ Bite saturation — 4-stage presence-weighted harmonic algorithm
3. ✅ GR Overshoot — VCA punch (internal, no user parameter)
4. ✅ Noise Gate — threshold knob (address 14)
5. ✅ Grip — dual-layer Peak/RMS detection (address 8)
6. ⬜ Input/Output level metering
7. ⬜ Preset system

### Sprint 3 (Planned)
1. De-esser
2. Mid/Side processing

---

## Architecture Notes

- All DSP is in `VX1ExtensionDSPKernel.hpp` (C++ class, render-thread safe)
- All parameters use `VX1ExtensionParameterAddress` enum (C header shared with Swift via bridging)
- UI uses SwiftUI with `ParameterKnob`, `VUMeter`, `BypassButton` components
- 60Hz meter timer in `VX1ExtensionAudioUnit.swift`
- All filter coefficients computed in `initialize()` — correct across 44.1/48/96 kHz
- **11 parameters** (addresses 0–14, several reserved); gainReductionMeter (address 11) is read-only

---

## Design Decisions Log

- **Hard knee** (fixed): no soft knee parameter. Instant compression at threshold = punchy, assertive character.
- **inputGain removed**: was functionally identical to lowering threshold. Redundant on a character compressor.
- **Grip default = 0%** (RMS): musical and smooth out of the box. Peak mode is opt-in for aggressive character.
- **Bite drive ceiling = 5x**: 8x caused aliasing/intermodulation artifacts ("wobbly distortion") above 50% on vocals. 5x is aggressive without artifacts.
- **Bite cubic grit self-limits**: `(1 - blend*0.5)` taper prevents x³ blowup when input is already heavily clipped at high drive.
- **Bite gain compensation = `1/tanh(drive×1.3)`**: exact mathematical normalization. Previous `1/(0.5+0.5×blend)` approximation was tuned for 4x drive and broke when drive was increased to 8x.
- **Grip knob direction**: 0% = left (RMS/smooth), 100% = right (Peak/tight). "Tight pull right" maps to aggressive compression.
- **Bite renamed from Sheen**: "Sheen" implied gentle shimmer. "Bite" better communicates the aggressive presence character at higher settings.
- **Grip renamed from Detect/Detection**: "Detection" was technical jargon. "Grip" is physical and intuitive — how hard the compressor holds onto the signal.
