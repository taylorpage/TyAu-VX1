# VX1 Compressor - User Guide

## Overview

The VX1 is a professional vocal compressor plugin designed to deliver aggressive, punchy, JJP-style compression with harmonic enhancement and parallel processing capabilities. It combines modern digital precision with analog-inspired warmth.

---

## Signal Flow

```
Input × InputGain → Detection (HPF → envelope) → GR + Overshoot → Saturation → Makeup → Mix → Output
```

**Features:**
- **Input Gain**: Drives signal harder into the compressor for "slammed" compression character
- **Sidechain HPF**: Fixed 80 Hz high-pass filter on detection path (always on) — prevents kick/bass from pumping
- **GR Overshoot**: VCA-style transient punch — brief extra grab on fast transients
- **Visual Metering**: Gain reduction meter with smooth ballistics, 60 Hz updates

---

## Parameters

### Gain Staging

#### **Input** (0 dB to +24 dB)
- **What it does**: Amplifies the signal before it reaches the compressor
- **Default**: 0 dB
- **How to set it**:
  - 0 dB = no pre-gain, standard signal level
  - Increasing Input drives a hotter signal into the compressor, forcing more gain reduction at any threshold/ratio setting
  - This is how hardware engineers "slam" a compressor — push a hot signal into a tight threshold
- **The key insight**: Input Gain affects BOTH the detection path and the audio path, so the compressor genuinely reacts to the louder signal (not a fake level boost after compression)
- **Sweet spots**:
  - **0 dB**: Standard, controlled compression
  - **+6 to +12 dB**: Noticeably more "grab" at the same threshold/ratio settings
  - **+18 to +24 dB**: Maximum slam — aggressive limiting territory

---

### Core Compression

#### **Threshold** (-50 dB to 0 dB)
- **What it does**: Sets the level at which compression begins
- **Default**: -20 dB
- **How to set it**:
  - Start at -20 dB and adjust until you see consistent gain reduction
  - Lower = more compression (compresses quieter sounds)
  - Higher = less compression (only compresses louder peaks)
- **Sweet spots**:
  - **Transparent**: -15 to -10 dB (light compression on peaks only)
  - **Moderate**: -25 to -20 dB (balanced vocal compression)
  - **Aggressive**: -40 to -30 dB (JJP-style heavy compression)

#### **Ratio** (1:1 to 30:1)
- **What it does**: Determines how much the signal is reduced above the threshold
- **Default**: 4:1
- **How to set it**:
  - 1:1 = no compression (unity)
  - 4:1 = for every 4 dB over threshold, reduce to 1 dB
  - Higher ratios = more aggressive compression
- **Sweet spots**:
  - **Transparent**: 2:1 to 3:1
  - **Moderate**: 4:1 to 6:1 (standard vocal compression)
  - **Aggressive**: 8:1 to 15:1 (JJP-style)
  - **Limiting**: 20:1 to 30:1 (brick wall)

---

### Dynamics Control

#### **Attack** (0 ms to 200 ms)
- **What it does**: How quickly the compressor responds to signals above threshold
- **Default**: 10 ms
- **How to set it**:
  - Faster = catches transients immediately (more control)
  - Slower = lets transients through (more punch)
- **Sweet spots**:
  - **Fast/Aggressive**: 0-5 ms (tight control, JJP-style)
  - **Balanced**: 10-30 ms (natural compression)
  - **Slow/Punchy**: 50-100 ms (preserves transients)

#### **Release** (5 ms to 5000 ms)
- **What it does**: How quickly compression stops after signal drops below threshold
- **Default**: 100 ms
- **How to set it**:
  - Faster = compression releases quickly (more pumping)
  - Slower = smooth, gradual release (more transparent)
- **Sweet spots**:
  - **Fast**: 50-150 ms (tight, controlled)
  - **Medium**: 200-500 ms (natural, musical)
  - **Slow**: 1000-3000 ms (smooth, leveling)
  - **Auto-style**: Match to tempo (120 BPM ≈ 500 ms)

---

### Character & Color

#### **Detection** (0% to 100%)
- **What it does**: Blends between Peak and RMS detection modes
- **Default**: 100% (pure RMS)
- **How it works**:
  - 0% = Pure Peak (instant response to every transient)
  - 100% = Pure RMS (responds to average loudness)
  - Middle values blend both
- **Sweet spots**:
  - **Peak-heavy (0-30%)**: Aggressive, catches every transient
  - **Balanced (40-60%)**: Mix of punch and musicality (JJP-style)
  - **RMS-heavy (70-100%)**: Smooth, average-based compression

#### **Sheen** (0% to 100%)
- **What it does**: Applies a 4-stage presence-weighted harmonic saturation algorithm
- **Default**: 25%
- **Algorithm**: Four-stage pipeline that concentrates harmonic generation in the 3–8 kHz presence/air band:
  1. Pre-emphasis shelf boosts 3.5 kHz+ before wave shaping → presence harmonics
  2. Asymmetric tanh wave shaper (DC offset) → even harmonics (2nd, 4th) for shimmer
  3. Cubic grit layer (x³) → 3rd harmonic edge for consonant cut-through
  4. De-emphasis shelf restores tonal balance; generated harmonics survive
- **Sweet spots**:
  - **Clean (0-15%)**: Minimal saturation, pristine
  - **Subtle presence (15-25%)**: Adds forward quality without obvious distortion
  - **JJP shine (35-50%)**: Bright, aggressive, vocal cuts through the mix
  - **Aggressive shine (50-65%)**: Very forward, works well with Mix at 50-70%
  - **Heavy/Creative (70-100%)**: Maximum coloration, intentional effect

---

### Level & Blend

#### **Makeup Gain** (-20 dB to +50 dB)
- **What it does**: Compensates for volume loss from compression
- **Default**: 0 dB
- **How to set it**:
  - Match output level to input for A/B comparison
  - Typically add 3-10 dB for moderate compression
  - Higher compression needs more makeup gain

#### **Mix** (0% to 100%)
- **What it does**: Parallel compression - blends dry and processed signals
- **Default**: 100% (fully wet)
- **How it works**:
  - 0% = completely dry (no compression heard)
  - 100% = fully compressed signal
  - Middle values = New York-style parallel compression
- **Sweet spots**:
  - **Serial (100%)**: Traditional compression
  - **Heavy parallel (50-70%)**: JJP-style punch
  - **Subtle parallel (20-40%)**: Adds thickness without obvious compression
  - **Dry (0%)**: Bypass (use Bypass button instead)

---

### Visual Feedback

#### **Gain Reduction Meter**
- **What it shows**: Real-time gain reduction in dB
- **Location**: Top of plugin, below title
- **Color coding**:
  - **Green (0-5 dB)**: Light compression, transparent
  - **Yellow (5-15 dB)**: Moderate compression, JJP sweet spot
  - **Orange (15-25 dB)**: Heavy compression, aggressive
  - **Red (25+ dB)**: Extreme compression, limiting territory
- **Meter ballistics**: Fast attack (responds quickly) + slow release (easy to read)
- **Target zones**:
  - **Transparent**: 2-5 dB
  - **JJP-style**: 8-15 dB (the "pocket")
  - **Aggressive**: 15-20 dB
  - **Limiting**: 20+ dB

**Note**: Meter is fully working with smooth ballistics (fast attack, adaptive release). Updates at 60 Hz.

---

## Preset Starting Points

### 1. JJP-Style Aggressive Vocal
**Goal**: Bright, punchy, upfront vocal that cuts through dense mixes

```
Threshold:    -35 dB
Ratio:        10:1
Attack:       2 ms
Release:      150 ms
Detection:    40% (peak-heavy)
Sheen:        50%
Makeup Gain:  +5 dB
Mix:          60%
```

**Why it works**: Fast attack catches transients, heavy ratio controls dynamics, peak detection adds aggression, Sheen adds presence harmonics, parallel mix retains punch.

**JJP Secret**: The combination of 50% Sheen + 60% parallel mix creates the signature "thick sheen" without sounding over-processed.

---

### 2. Transparent Vocal Polish
**Goal**: Subtle control that doesn't sound compressed

```
Threshold:    -15 dB
Ratio:        3:1
Attack:       30 ms
Release:      300 ms
Detection:    80% (RMS-heavy)
Sheen:        15%
Makeup Gain:  +4 dB
Mix:          100%
```

**Why it works**: Moderate threshold, gentle ratio, slow attack to let transients breathe, RMS detection for natural feel, minimal Sheen keeps the sound transparent.

---

### 3. Thick Parallel Compression
**Goal**: Add body and presence without changing dynamics

```
Threshold:    -30 dB
Ratio:        8:1
Attack:       5 ms
Release:      100 ms
Detection:    50%
Sheen:        35%
Makeup Gain:  0 dB
Mix:          40%
```

**Why it works**: Heavy compression on wet signal blended subtly with dry creates thickness without losing transients.

**Pro Tip**: The 40% mix keeps the natural dynamics while adding controlled density.

---

### 4. Rap/Hip-Hop Vocal
**Goal**: Aggressive, in-your-face, heavily processed

```
Threshold:    -40 dB
Ratio:        15:1
Attack:       1 ms
Release:      80 ms
Detection:    20% (peak-dominant)
Sheen:        60%
Makeup Gain:  +15 dB
Mix:          70%
```

**Why it works**: Very fast attack controls every syllable, high ratio for consistent level, peak detection for aggression, high Sheen for edge and presence.

---

### 5. Ballad/Smooth Vocal
**Goal**: Even dynamics without harshness

```
Threshold:    -25 dB
Ratio:        4:1
Attack:       20 ms
Release:      800 ms
Detection:    90% (RMS)
Sheen:        20%
Makeup Gain:  +6 dB
Mix:          100%
```

**Why it works**: Gentle compression with slow release smooths performance, high RMS detection for natural feel, minimal Sheen keeps it clean.

---

## Usage Tips

### General Workflow

1. **Set Input Gain**: Start at 0 dB; increase to +6–12 dB to "slam" the compressor
2. **Set Threshold & Ratio**: Get the basic compression working (watch the meter)
3. **Adjust Attack/Release**: Shape the compression response
4. **Tune Detection**: Find the right balance of punch vs. smoothness
5. **Add Sheen**: Introduce harmonic enhancement
6. **Set Mix**: Decide between serial or parallel compression
7. **Set Makeup Gain**: Adjust to compensate for gain reduction
8. **Check the Meter**: Aim for 8-15 dB GR for JJP-style compression

### Common Mistakes to Avoid

- **Too much compression + too much drive**: Creates harsh, pumping artifacts
- **Attack too fast with 100% mix**: Kills transients, sounds lifeless
- **Release too short**: Creates obvious pumping
### A/B Comparison Tips

1. Set Makeup Gain to match perceived loudness
2. Use Bypass button to compare processed vs. unprocessed
3. Lower Mix to 0% instead of bypassing to hear just the wet signal

---

## Technical Details

### Detection Algorithm
- Calculates both Peak (absolute maximum) and RMS (average energy) per sample
- Blends based on Detection parameter
- Feeds into exponential envelope follower with separate attack/release

### Compression Curve
- Hard knee (fixed): Instant compression at threshold — gain reduction begins the moment the signal exceeds threshold
- Formula above threshold: `gainReductionDb = overThresholdDb * (1 - 1/ratio)`

### Sheen Saturation Algorithm
- 4-stage presence-weighted harmonic saturation
- Stage 1: Pre-emphasis +5 dB high shelf @ 3.5 kHz → biases harmonic generation to presence band
- Stage 2: Asymmetric tanh wave shaper with DC offset → even harmonics (shimmer/sparkle)
- Stage 3: Cubic grit (`x³` at 6% blend) → 3rd harmonic edge in presence/air band
- Stage 4: De-emphasis −5 dB shelf restores fundamental balance; harmonics survive
- Gain compensation: `1 / (0.5 + 0.5×blend)` keeps wet signal audible at all Sheen settings
- See `Docs/VX1_Saturation_Algorithm.md` for full mathematical documentation

### GR Overshoot (VCA Punch)
- When a transient causes GR to increase >3 dB in one sample, an additional 3 dB of GR is applied
- Hold: ~0.5ms (sample-rate adaptive)
- Release: exponential decay back to normal over ~2ms
- Replicates the physical overshoot behavior of dbx 160 / SSL G-bus VCA gain cells
- Creates the "slammed grab" on transients that defines aggressive compressor character

### Parallel Processing
- Dry signal preserved completely
- Compression → Saturation → Makeup applied to wet path
- Final mix: `output = (dry * (1 - mix)) + (wet * mix)`

---

## The JJP Signature Sound Formula

The VX1 achieves the classic JJP vocal sound through a specific combination of features:

### Core Recipe:
1. **Input Gain +6–12 dB**: Drive the signal into the compressor for more grab
2. **Heavy Compression**: 8-15 dB of gain reduction (check the meter)
3. **Parallel Mix**: 50-70% wet (retains transients while adding density)
4. **Sheen at 40-60%**: Adds presence-weighted harmonic "shine"
5. **Peak Detection (20-50%)**: Adds aggression and attack

### Why It Works:
- **Input Gain**: Drives a hotter signal into the compressor → more GR at the same settings → the "slammed" grab character.
- **Parallel compression**: Creates the "thick but punchy" paradox — density without losing dynamics.
- **Sheen + Parallel**: 4-stage presence harmonics on the wet signal only = shimmer and character without distorting the full mix.
- **GR Overshoot**: Automatic VCA-style transient grab — the extra "punch" that makes fast transients feel physical.

### Quick JJP Setup:
1. Set Input Gain to +6–9 dB for extra slam
2. Set threshold for 10-12 dB gain reduction
3. Set Mix to 60%
4. Set Sheen to 50%
5. Adjust Detection to taste (40% is a good start)
6. Set Makeup Gain to match your output level

---

## Parameter Interactions

### Threshold + Ratio
- Low threshold + low ratio = gentle leveling
- Low threshold + high ratio = heavy compression
- High threshold + high ratio = peak limiting only

### Attack + Detection
- Fast attack + Peak detection = aggressive transient control
- Slow attack + RMS detection = smooth, natural compression

### Sheen + Mix
- High Sheen + low mix = adds presence character without over-processing
- Low Sheen + high mix = clean compression
- High Sheen + high mix = maximum aggression (use carefully!)

### Input Gain + Threshold
- Increasing Input Gain is equivalent to moving threshold down — both force more GR
- Use Input Gain when you want more slam without changing the threshold/ratio relationship
- At high Input Gain, watch the meter and adjust Makeup Gain to compensate for level changes

---

