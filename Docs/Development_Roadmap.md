# VX1 Compressor - Development Roadmap

## Current Status

### Implemented Features

1. **Core Compression Engine**
   - Feed-forward compression with gain reduction
   - Adjustable threshold (-50 to 0 dB)
   - Wide ratio range (1:1 to 30:1)
   - Attack (0-200 ms) and Release (5-5000 ms) controls

2. **Advanced Compression Features**
   - Hard knee (fixed) — instant compression at threshold for punchy, assertive character
   - Parallel compression via Mix control (0-100%)
   - **Grip** — Peak/RMS detection blend with dual-layer envelope interaction
     - 0% = RMS mode: 175ms IIR window, uses attack knob time — smooth, musical
     - 100% = Peak mode: 2ms near-instant envelope grab — aggressive, slammy
     - Both the detected level AND the envelope attack coefficient are blended

3. **Character & Enhancement**
   - **Bite saturation** — 4-stage presence-weighted harmonic algorithm
   - Even harmonics via asymmetric tanh wave shaper (DC offset → 2nd harmonic)
   - Odd harmonics via cubic grit layer (x³ → 3rd harmonic, self-limiting at high drive)
   - Drive: 1x–5x, DC offset: 0–0.18, gain compensation: `1/tanh(drive×1.3)` (exact normalization)

4. **Level Control**
   - Manual makeup gain (-20 to +50 dB)
   - Bypass functionality

5. **Visual Feedback**
   - Gain reduction meter with smooth ballistics
   - Color-coded display (green → yellow → orange → red)
   - 60 Hz meter updates
   - Meter reflects overshoot GR for accurate visual match to what you hear

6. **DSP Stability & Character**
   - Clean single-pole envelope follower with no abrupt resets
   - GR Overshoot / VCA Punch (internal, no parameter) — VCA-style transient grab on fast attacks
   - Noise Gate — pre-compression, prevents noise floor amplification between phrases

---

## Parameter List (11 parameters, addresses 0–14)

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

> Addresses 7, 10, 12, 13 are reserved/removed. Address 7 = knee (removed, hard-coded). Address 10 = autoMakeup (removed). Address 12 = lookAhead (removed). Address 13 = inputGain (removed — redundant with threshold on a character compressor).

---

## Signal Flow (Current)

```
Input
  │
  ├─[Noise Gate]
  │   raw input → peak envelope follower (0.5ms attack, 50ms hold, 100ms release)
  │   → compare to gateThreshold → mGateGain scalar (0=closed, 1=open)
  │
  ├─[Sidechain — detection only]
  │   input × mGateGain → mono sum
  │   → fixed 80 Hz 2-pole Butterworth HPF
  │   → peak (instantaneous abs) and RMS (175ms IIR accumulator)
  │   → blended by Grip: 0%=RMS, 100%=Peak
  │   → envelope follower: attack blended by Grip (2ms→user attack), release fixed
  │   → GR calculation (threshold, ratio, hard knee)
  │   → GR Overshoot check (VCA punch)
  │
  ├─[Audio path]
  │   input × mGateGain → audioInput
  │   → gainReductionTotal applied (GR + overshoot)
  │   → Bite saturation (4-stage algorithm)
  │   → makeup gain
  │   → parallel mix (dry/wet blend)
  │
  └─ Output
```

---

## DSP Design Notes

### Grip — Dual-Layer Detection
The Grip knob does two things simultaneously:
1. Blends the detected level between RMS (175ms window, responds to energy) and Peak (instantaneous)
2. Blends the envelope follower's attack coefficient between the user's attack knob and a fixed 2ms "instant" attack

At Grip=0% (RMS), the compressor cannot grab a transient faster than the attack knob allows. At Grip=100% (Peak), it grabs in 2ms regardless of the attack setting. The combined effect is dramatic and audible across the full range.

### Bite — Presence-Biased Harmonic Saturation
Four-stage algorithm runs post-compression:
1. **Pre-emphasis**: +5 dB shelf @ 3.5 kHz — biases harmonic generation toward presence/air band
2. **Asymmetric wave shaper**: tanh with DC offset (0–0.18) — generates 2nd harmonic (octave shimmer)
3. **Cubic grit**: x³ scaled by `0.06 × blend × (1 - blend×0.5)` — self-limiting at high drive to prevent intermodulation
4. **De-emphasis**: matching -5 dB shelf — restores tonal balance; harmonics survive above corner
5. **Gain compensation**: `1/tanh(drive×1.3)` — exact normalization at any drive setting

Drive range: 1x (0%) → 5x (100%). The cubic grit term scales back in the upper knob range to prevent aliasing artifacts.

### GR Overshoot / VCA Punch
When GR jumps >3 dB in one sample: +3 dB extra GR applied for 0.5ms hold, then exponentially released over 2ms. Replicates VCA gain cell physical overshoot (dbx 160 / SSL G-bus character).

---

## UI Layout

Window: 350×580px, dark gradient background.

```
[Title: VX1 COMPRESSOR]
[LED indicator]
[Gain Reduction Meter]
Row 1: [GATE 55px]  [THRESHOLD 55px]  [RATIO 55px]  (spacing 10px, GATE label is orange)
Row 2: [ATTACK 65px]    [RELEASE 65px]
Row 3: [MIX 65px]       [GRIP 65px]
Row 4: [MAKEUP 65px]    [BITE 65px]
[BYPASS button]
[TaylorAudio logo]
```

---

## Roadmap

### Phase 2 — Remaining
- Input/Output level metering
- Preset system

### Phase 3 — Planned
- De-esser (multiband, 4–10 kHz)
- Mid/Side processing

### Phase 4 — Polish
- 2x/4x oversampling (reduces Bite aliasing at extreme settings)
- Additional UI refinements

---

## Key Files

- **DSP**: `VX1Extension/DSP/VX1ExtensionDSPKernel.hpp`
- **Parameters**: `VX1Extension/Parameters/Parameters.swift`
- **Parameter Addresses**: `VX1Extension/Parameters/VX1ExtensionParameterAddresses.h`
- **UI**: `VX1Extension/UI/VX1ExtensionMainView.swift`
- **Session State**: `Docs/Session_Context.md`

---

## Testing Checklist

### Before Each Release
- [ ] Test at 44.1 kHz, 48 kHz, 96 kHz
- [ ] Test mono and stereo
- [ ] Verify no audio dropouts or glitches
- [ ] Check parameter automation works smoothly
- [ ] A/B against reference compressors (JJP Vocals, CLA-76)
- [ ] Test extreme parameter settings (Bite 100%, Grip 100%, etc.)
- [ ] Verify bypass works correctly
- [ ] Confirm meter shows overshoot spikes on fast transients
- [ ] Confirm Grip knob sweep is audibly smooth and linear end-to-end
- [ ] Confirm Bite knob sweep is audibly smooth with no wobble or artifacts
