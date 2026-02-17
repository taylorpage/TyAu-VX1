# VX1 Compressor - Development Roadmap

## Current Status

### ✅ Implemented Features

1. **Core Compression Engine**
   - Feed-forward compression with gain reduction
   - Adjustable threshold (-50 to 0 dB)
   - Wide ratio range (1:1 to 30:1)
   - Attack (0-200 ms) and Release (5-5000 ms) controls

2. **Advanced Compression Features**
   - Soft knee (0-12 dB) with quadratic interpolation
   - Parallel compression via Mix control (0-100%)
   - Peak/RMS detection blend (0-100%)

3. **Character & Enhancement**
   - **Sheen saturation** — 4-stage presence-weighted harmonic algorithm (see below)
   - Even and odd harmonic generation via asymmetric wave shaper + cubic grit layer
   - Fixed gain compensation formula for audible character at all settings

4. **Level Control**
   - **Input Gain** (0 to +24 dB) — pre-compression signal drive ✨ NEW
   - Manual makeup gain (-20 to +50 dB)
   - **Auto makeup gain** (toggleable, compensates 80% of average GR)
   - Bypass functionality

5. **Visual Feedback**
   - **Gain reduction meter** with smooth ballistics ✅ FULLY WORKING
   - Color-coded display (green → yellow → orange → red)
   - 60 Hz meter updates for responsive feedback
   - Meter reflects overshoot GR for accurate visual match to what you hear

6. **DSP Stability & Character**
   - Clean single-pole envelope follower with no abrupt resets
   - **GR Overshoot / VCA Punch** (internal, no parameter) ✨ NEW — VCA-style transient grab on fast attacks
   - Fixed host app component lookup

---

## Parameter List (14 parameters, addresses 0–13)

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

---

## Roadmap to JJP-Style Compression

### Phase 1: Core Enhancement (High Priority) - Sprint 1 ✅ COMPLETE

#### 1.1 Auto Makeup Gain ✅
- Tracks average GR with 100ms smoothing, adds 80% of average GR as makeup
- Toggle switch below Makeup knob
- Works multiplicatively with manual makeup gain

#### 1.2 Look-Ahead ✅
- Fixed-step control: Off / 2ms / 5ms / 10ms
- Ring buffer allocated once at `initialize()`, never mid-stream
- Host latency reported via `AUAudioUnit.latency`

#### 1.3 High-Pass Filter in Sidechain ✅
- Fixed 80 Hz cutoff — no parameter, always active
- 2-pole Butterworth (12 dB/oct), bilinear-transform design
- Runs on a mono sum of the input before the envelope follower
- Prevents kick/bass energy from pumping vocal compression

---

### Phase 2: Professional Features - Sprint 2

#### 2.1 Sheen Saturation ✅
**Status**: IMPLEMENTED (renamed from "Drive" in last session)

**4-stage presence-weighted harmonic saturation**:

1. **Pre-emphasis**: +5 dB high shelf @ 3.5 kHz before wave shaper — forces harmonics to be generated in the presence/air band
2. **Asymmetric wave shaper**: `tanh` with DC offset → positive half-cycles clip harder → even harmonics (2nd, 4th) for shimmer/sheen quality
3. **Cubic grit layer**: `x³` at 6% blend → 3rd harmonic edge (4–12 kHz for vocal fundamentals)
4. **De-emphasis**: Matching −5 dB shelf cut restores tonal balance; generated harmonics survive above the corner frequency

**Gain compensation fix**: `1 / (0.5 + 0.5×blend)` — the old `(1/drive)×1.2` formula buried the wet signal ~10 dB at Sheen=100%.

**Why this sounds like JJP**:
- Presence-weighted harmonics concentrated in 3–8 kHz, not the low-mid
- Even harmonic shimmer (DC offset asymmetry → 2nd harmonic)
- Controlled grit that makes consonants cut through
- Character audible at all settings (fixed compensation)

See `Docs/VX1_Saturation_Algorithm.md` for full algorithm documentation.

---

#### 2.2 Input Gain ✅ NEW
**Status**: IMPLEMENTED (address 13)

**What it does**: Pre-compression input gain knob, 0 to +24 dB, default 0 dB.

**Why it matters**:
- Drives more signal into the compressor, forcing more GR at any threshold/ratio setting
- This is how engineers "slam" a compressor — hot signal into a tight threshold
- Applied to BOTH the sidechain detection path AND the audio path simultaneously
  - The compressor reacts to the hotter signal (correct behavior)
  - Output level is automatically managed via auto makeup or manual makeup gain

**Implementation**:
- Applied in `process()`: `monoSC += inputBuffers[channel][frameIndex] * mInputGainLinear;`
- And: `float currentInput = inputBuffers[channel][frameIndex] * mInputGainLinear;`
- Linear gain cached: `mInputGainLinear = std::pow(10.0f, mInputGainDb / 20.0f);`

**UI**: INPUT knob in Row 1 alongside Threshold and Ratio (60px, cyan label)

---

#### 2.3 GR Overshoot / VCA Punch ✅ NEW
**Status**: IMPLEMENTED (internal — no user parameter)

**What it does**: When a transient causes GR to jump by >3 dB in a single sample, briefly over-applies an additional 3 dB of GR for ~0.5ms, then exponentially releases the extra GR back to the normal level over ~2ms.

**Why this matters**: VCA gain cells (dbx 160, SSL G-bus) physically overshoot their target GR on fast transients before the feedback loop settles. This creates the "slammed" grab on transients that defines aggressive compressor character. The VX1 now replicates this behavior digitally.

**Internal parameters (computed in `initialize()`)**:
- Trigger: GR increases >3 dB in one sample
- Overshoot amount: +3 dB extra GR during hold
- Hold time: 0.5ms (`mOvershootHoldSamples = 0.0005f × sampleRate`)
- Release coefficient: `exp(-1 / (0.002f × sampleRate))` → ~2ms exponential decay

**Meter**: Shows `totalGainReductionDb` (includes overshoot) so the meter accurately matches what the listener hears.

---

#### 2.4 Input/Output Metering
**Status**: Not implemented
- Add input level meter and output level meter
- Show peak and RMS
- Benefits: prevent clipping, monitor levels, professional workflow

#### 2.5 Presets System
**Status**: Not implemented
- Factory presets (JJP Style, Transparent, Thick, etc.)
- User preset save/load
- Benefits: faster workflow, good starting points, marketing/demo value

---

### Phase 3: Advanced Features - Sprint 3

#### 3.1 De-esser (Multiband on High Frequencies) ⭐⭐
**Status**: Not implemented
- Split signal at ~4-8 kHz
- Apply separate compression to high band
- Parameters: De-ess Amount (0-100%), De-ess Frequency (4-10 kHz)
- JJP Vocals has built-in de-essing

#### 3.2 Mid/Side Processing ⭐
**Status**: Not implemented
- M/S encoding/decoding
- Separate threshold for Mid and Side
- Parameter: M/S Link (0-100%)

#### 3.3 RMS Window Size Control ⭐
**Status**: Currently per-sample RMS
- Add sliding RMS window (1-20 ms)
- Parameter: RMS Window (Fast 1ms / Medium 5ms / Slow 10ms)

---

### Phase 4: Polish & Refinement

#### 4.1 Oversampling ⭐
**Status**: Not implemented
- 2x or 4x oversampling option
- Apply before saturation, downsample after
- Parameter: Oversampling (Off / 2x / 4x)
- Reduces aliasing in saturation stage

#### 4.2 Additional Preset Modes
#### 4.3 UI Refinements

---

## Signal Flow (Current)

```
Input
  │
  ├─[Sidechain — detection only]
  │   input × inputGainLinear → mono sum
  │   → fixed 80 Hz 2-pole Butterworth HPF
  │   → abs(filtered) → envelope follower (attack/release)
  │   → GR calculation (threshold, ratio, soft knee)
  │   → GR Overshoot check (VCA punch)
  │
  ├─[Audio path]
  │   input × inputGainLinear → currentInput
  │   → ring buffer delay (look-ahead: 0/2/5/10ms)
  │   → gainReductionTotal applied (GR + overshoot)
  │   → Sheen saturation (4-stage algorithm)
  │   → makeup gain (manual × auto)
  │   → parallel mix (dry/wet blend)
  │
  └─ Output
```

---

## JJP Vocal Feature Comparison

### What JJP Vocals Has:
1. ✅ Compression (we have this)
2. ✅ Saturation/Attitude (Sheen — 4-stage harmonic algorithm)
3. ✅ Parallel processing (Mix)
4. ✅ Auto makeup gain (80% compensation)
5. ✅ Input drive / signal slamming (Input Gain)
6. ✅ VCA transient grab (GR Overshoot)
7. ❌ De-esser (Sprint 3)
8. ✅ Sidechain filtering (80 Hz HPF, always on)
9. ⚠️ Multiple preset modes (planned — Sprint 2)
10. ❌ Reverb/Space (out of scope for compressor)
11. ❌ Pitch correction (out of scope)

### What We Have That JJP Doesn't:
1. ✅ Adjustable soft knee (0-12 dB)
2. ✅ Peak/RMS detection blend
3. ✅ Wider parameter ranges
4. ✅ Visual gain reduction metering
5. ✅ Look-ahead with host latency reporting

---

## Sprint Status

### Sprint 1 ✅ COMPLETE
1. ✅ Auto Makeup Gain
2. ✅ Gain Reduction Meter
3. ✅ DSP Stability (no pops)
4. ✅ Look-Ahead with ring buffer + host latency

### Sprint 2 (In Progress)
1. ✅ Sidechain HPF — fixed 80 Hz, always on
2. ✅ Sheen Saturation — 4-stage JJP presence algorithm
3. ✅ Input Gain — pre-compression drive (address 13)
4. ✅ GR Overshoot — VCA punch (internal)
5. ⬜ Input/Output Metering
6. ⬜ Presets System

### Sprint 3 (Planned)
1. De-esser
2. RMS Window Control
3. Mid/Side Processing

---

## Architecture Notes

- All DSP is in `VX1Extension/DSP/VX1ExtensionDSPKernel.hpp` (C++ class, render-thread safe)
- All parameters use `VX1ExtensionParameterAddress` enum (C header shared with Swift via bridging)
- UI uses SwiftUI with `ParameterKnob`, `GainReductionMeter`, `BypassButton` components
- 60Hz meter timer in `VX1ExtensionAudioUnit.swift`
- Host latency reported via `latency` override in same file
- Ring buffer always allocated at max capacity (never mid-stream realloc)
- All filter coefficients computed in `initialize()` — correct across 44.1/48/96 kHz
- **14 parameters** (addresses 0–13); gainReductionMeter is read-only

---

## Current UI Layout

Window: 350×580px, dark gradient background.

```
[Title: VX1 COMPRESSOR]
[LED indicator]
[Gain Reduction Meter]
Row 1: [INPUT 60px]     [THRESHOLD 60px]  [RATIO 60px]   (spacing 12px)
Row 2: [ATTACK 65px]    [RELEASE 65px]
Row 3: [MIX 65px]       [KNEE 65px]       [DETECT 65px]
Row 4: [MAKEUP 65px]    [SHEEN 65px]
       └[AUTO toggle]
Row 5: [LOOK-AHEAD segmented: Off / 2ms / 5ms / 10ms]
[BYPASS button]
[TaylorAudio logo]
```

---

## Key Files

- **DSP**: `VX1Extension/DSP/VX1ExtensionDSPKernel.hpp`
- **Parameters**: `VX1Extension/Parameters/Parameters.swift`
- **Parameter Addresses**: `VX1Extension/Parameters/VX1ExtensionParameterAddresses.h`
- **UI**: `VX1Extension/UI/VX1ExtensionMainView.swift`
- **Algorithm Docs**: `Docs/VX1_Saturation_Algorithm.md`
- **Session State**: `Docs/Session_Context.md`

---

## Performance Considerations

### Current Performance
- Per-sample processing (no SIMD yet)
- Simple algorithms (tanh, sqrt, log10, pow)
- Runs efficiently on modern CPUs

### Optimization Opportunities
1. SIMD vectorization for multi-channel processing
2. Look-up tables for tanh saturation
3. Parallel processing for independent channels
4. Lazy parameter updates (only recalculate when changed)

---

## Testing Checklist

### Before Each Release
- [ ] Test at 44.1 kHz, 48 kHz, 96 kHz
- [ ] Test mono and stereo operation
- [ ] Verify no audio dropouts or glitches
- [ ] Check parameter automation works smoothly
- [ ] A/B against reference compressors (JJP Vocals, CLA-76, etc.)
- [ ] Test extreme parameter settings (Input +24 dB, Sheen 100%, etc.)
- [ ] Verify bypass works correctly
- [ ] Check CPU usage is reasonable
- [ ] Confirm meter shows overshoot spikes on fast transients

---

## Long-Term Vision

### Version 1.0 (Current + Sprint 2)
- Solid vocal compressor with full JJP-style feature set
- Input drive, VCA punch, 4-stage Sheen, auto makeup, look-ahead, metering
- Preset system with good factory presets

### Version 1.5 (+ Sprint 3)
- De-esser for complete vocal processing
- M/S processing for stereo control
- Professional-grade feature set

### Version 2.0 (Future)
- Multi-band compression option
- Advanced metering and visualization
- Multiple compressor modes (VCA, Opto, FET, etc.)
- External sidechain input

---

## Resources & References

### Inspiration
- Waves JJP Vocals
- CLA-76 (fast attack compressor)
- SSL Bus Compressor (parallel compression pioneer)
- LA-2A (smooth optical compression)
- dbx 160 (VCA design, transient overshoot behavior)

### Technical References
- [DAFX - Digital Audio Effects](https://www.dafx.de/) - Compression algorithms
- [Julius O. Smith - Audio DSP](https://ccrma.stanford.edu/~jos/) - Envelope followers
- [Designing Audio Effect Plugins in C++](https://www.willpirkle.com/) - Plugin architecture
