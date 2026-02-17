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
   - Tape-style saturation with Drive control (0-100%)
   - Even harmonic enhancement for "sheen"
   - Automatic gain compensation in saturation

4. **Level Control**
   - Manual makeup gain (-20 to +50 dB)
   - **Auto makeup gain** (toggleable, compensates 80% of average GR) ✨ NEW
   - Bypass functionality

5. **Visual Feedback**
   - **Gain reduction meter** with smooth ballistics ✅ FULLY WORKING
   - Color-coded display (green → yellow → orange → red)
   - 60 Hz meter updates for responsive feedback

6. **DSP Stability Fixes** ✅
   - Removed mid-stream state resets (was causing audio pops/glitches)
   - Clean single-pole envelope follower with no abrupt resets
   - Fixed host app component lookup (wrong subtype `gain` → `vx1c`)

---

## Roadmap to JJP-Style Compression

### Phase 1: Core Enhancement (High Priority) - Sprint 1

#### 1.1 Auto Makeup Gain ⭐⭐⭐
**Status**: ✅ IMPLEMENTED
**How it works**:
- Tracks average gain reduction using 100ms smoothing time constant
- Automatically adds makeup gain = 80% of average GR (intentional under-compensation for JJP-style punch)
- Toggle switch placed below Makeup knob for easy access
- Works multiplicatively with manual makeup gain for fine-tuning

**Why it achieves JJP sound**:
- The 80% compensation prevents over-leveling, maintaining dynamic contrast
- Combined with parallel mix, creates the signature "thick but punchy" JJP characteristic
- Faster workflow allows more focus on creative decisions

**Implementation Details**:
- Location: `VX1ExtensionDSPKernel.hpp` lines 211-226
- Parameter: `autoMakeup` (boolean, default: off)
- UI: Toggle below Makeup knob in `VX1ExtensionMainView.swift`

---

#### 1.2 Look-Ahead ⭐⭐
**Status**: ❌ REMOVED — not needed for vocal compression
**Decision**: Look-ahead was implemented and reverted. For vocals, the Detection (Peak/RMS blend) knob already provides equivalent transient control without the complexity and latency overhead. Look-ahead is more valuable on bus/drum compressors. Replaced in priority by Sidechain HPF.

---

#### 1.3 High-Pass Filter in Sidechain ⭐⭐
**Status**: Not implemented
**Why it matters**: Prevents bass from triggering compression on vocals
**Implementation**:
- Add sidechain high-pass filter (12 dB/oct)
- Parameter: Sidechain HPF (Off, 60, 80, 100, 120, 150 Hz)
- Apply only to detection path, not audio path

**Benefits**:
- More consistent compression on vocals
- Prevents bass/kick from pumping the vocal
- Common in professional vocal compressors

---

### Phase 2: Advanced Features (Medium Priority)

#### 2.1 De-esser (Multiband on High Frequencies) ⭐⭐
**Status**: Not implemented
**Why it matters**: JJP Vocals has built-in de-essing
**Implementation**:
- Split signal at ~4-8 kHz
- Apply separate compression to high band
- Parameter: De-ess Amount (0-100%)
- Parameter: De-ess Frequency (4-10 kHz)

**Benefits**:
- One-stop vocal processing
- Tames harsh sibilance
- Matches JJP workflow

---

#### 2.2 Mid/Side Processing ⭐
**Status**: Not implemented
**Why it matters**: Professional vocal compression often processes mid differently from sides
**Implementation**:
- Add M/S encoding/decoding
- Separate threshold for Mid and Side
- Parameter: M/S Link (0-100%)

**Benefits**:
- Better stereo image control
- Can compress center vocal without affecting stereo effects
- More professional feature set

---

#### 2.3 RMS Window Size Control ⭐
**Status**: Currently per-sample RMS
**Why it matters**: Adjustable RMS window affects compression character
**Implementation**:
- Add sliding RMS window (1-20 ms)
- Parameter: RMS Window (Fast 1ms / Medium 5ms / Slow 10ms)
- Currently calculating RMS per-sample (0 ms window)

**Benefits**:
- More control over compression feel
- Fast window = more responsive
- Slow window = smoother, less reactive

---

### Phase 3: Polish & Refinement (Lower Priority)

#### 3.1 Gain Reduction Metering ⭐⭐⭐
**Status**: ✅ FULLY WORKING
**What's working**:
- Visual meter component with gradient (green → yellow → orange → red)
- 60 Hz timer updates from AudioUnit
- Smooth ballistics (fast attack 30%, slow release 95%)
- Peak tracking across render buffer

**Implementation Details**:
- Location: `VX1ExtensionDSPKernel.hpp` (meter calculation in process loop)
- UI Component: `GainReductionMeter.swift`
- Timer: `VX1ExtensionAudioUnit.swift`

**Why it matters for JJP sound**:
- Visual feedback lets users dial in the "sweet spot" (8-15 dB GR for JJP-style)
- Helps match compression intensity across different tracks
- Professional appearance builds confidence in the plugin

---

#### 3.2 Input/Output Metering ⭐⭐
**Status**: Not implemented
**Implementation**:
- Add input level meter
- Add output level meter
- Show peak and RMS

**Benefits**:
- Prevent clipping
- Monitor levels
- Professional workflow

---

#### 3.3 Presets System ⭐⭐
**Status**: Not implemented
**Implementation**:
- Add preset browser in UI
- Include factory presets (JJP Style, Transparent, Thick, etc.)
- User preset save/load functionality

**Benefits**:
- Faster workflow
- Good starting points
- Marketing/demo value

---

#### 3.4 Oversampling ⭐
**Status**: Not implemented
**Why it matters**: Reduces aliasing in saturation stage
**Implementation**:
- 2x or 4x oversampling option
- Apply before saturation
- Downsample after processing
- Parameter: Oversampling (Off / 2x / 4x)

**Benefits**:
- Cleaner saturation
- Less aliasing artifacts
- More "analog" sound

---

## JJP Vocal Feature Comparison

### What JJP Vocals Has:
1. ✅ Compression (we have this)
2. ✅ Saturation/Attitude (we have Drive)
3. ✅ Parallel processing (we have Mix)
4. ✅ Auto makeup gain (IMPLEMENTED - 80% compensation)
5. ❌ De-esser (not implemented - Sprint 3)
6. ❌ Sidechain filtering (not implemented - Sprint 2)
7. ⚠️ Multiple preset modes (planned - Sprint 2)
8. ❌ Reverb/Space (out of scope for compressor)
9. ❌ Pitch correction (out of scope)

### What We Have That JJP Doesn't:
1. ✅ Adjustable soft knee (0-12 dB)
2. ✅ Peak/RMS detection blend (0-100%)
3. ✅ Wider parameter ranges (more flexibility)
4. ✅ More transparent mode options
5. ✅ Visual gain reduction metering (JJP has this, but ours will be customizable)

---

## Recommended Implementation Order

### Sprint 1: Essential JJP Features ✅ COMPLETE
1. ✅ Auto Makeup Gain — toggleable 80% GR compensation
2. ✅ Gain Reduction Meter — fully working with smooth ballistics
3. ✅ DSP Stability — clean envelope follower, no mid-stream pops
4. ❌ Look-Ahead — removed, not needed for vocal compression

### Sprint 2: Professional Features
1. Sidechain High-Pass Filter
2. Input/Output Metering
3. Presets System

### Sprint 3: Advanced Features
1. De-esser
2. RMS Window Control
3. Mid/Side Processing

### Sprint 4: Polish
1. Oversampling
2. Additional preset modes
3. UI refinements

---

## Current Architecture Notes

### Signal Processing Flow
```cpp
// Current flow (VX1ExtensionDSPKernel.hpp, lines 153-210)
1. Calculate Peak and RMS
2. Blend Peak/RMS based on Detection parameter
3. Envelope follower (Attack/Release)
4. Calculate gain reduction (Threshold, Ratio, Knee)
5. Apply compression
6. Apply saturation (Drive)
7. Apply makeup gain
8. Parallel mix (dry/wet blend)
```

### Key Files
- **DSP**: `VX1Extension/DSP/VX1ExtensionDSPKernel.hpp`
- **Parameters**: `VX1Extension/Parameters/Parameters.swift`
- **UI**: `VX1Extension/UI/VX1ExtensionMainView.swift`
- **Parameter Addresses**: `VX1Extension/Parameters/VX1ExtensionParameterAddresses.h`

### Parameter Count
- **Current**: 12 parameters (threshold, ratio, attack, release, makeupGain, bypass, mix, knee, detection, drive, autoMakeup, gainReductionMeter)
- **User-controllable**: 11 (gainReductionMeter is read-only for metering)
- **UI Layout**: Increased window height to 540px to accommodate meter
- **Space available**: Limited - may need tabbed interface for advanced features in Sprint 3+

---

## Performance Considerations

### Current Performance
- Per-sample processing (no SIMD yet)
- Simple algorithms (tanh, sqrt, log10)
- Should run efficiently on modern CPUs

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
- [ ] Test extreme parameter settings
- [ ] Verify bypass works correctly
- [ ] Check CPU usage is reasonable

---

## Long-Term Vision

### Version 1.0 (Current + Sprint 1-2)
- Solid vocal compressor with JJP-style features
- Auto makeup gain, look-ahead, metering
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

## Questions to Resolve

1. ✅ **Auto Makeup Gain**: RESOLVED - Toggleable with switch below knob
2. ✅ **Look-Ahead**: RESOLVED - Removed, not appropriate for vocal compressor
3. ✅ **Meter Debugging**: RESOLVED - Meter fully working
4. **Sidechain HPF**: Fixed frequencies or continuous sweep?
5. **De-esser**: Separate module or integrated into main compression?
6. **UI Space**: Window at 540px height - may need tabs for advanced features in Sprint 3+

---

## Resources & References

### Inspiration
- Waves JJP Vocals
- CLA-76 (fast attack compressor)
- SSL Bus Compressor (parallel compression pioneer)
- LA-2A (smooth optical compression)

### Technical References
- [DAFX - Digital Audio Effects](https://www.dafx.de/) - Compression algorithms
- [Julius O. Smith - Audio DSP](https://ccrma.stanford.edu/~jos/) - Envelope followers
- [Designing Audio Effect Plugins in C++](https://www.willpirkle.com/) - Plugin architecture

---

