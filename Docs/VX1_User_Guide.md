# VX1 Compressor - User Guide

## Overview

The VX1 is a professional vocal compressor plugin designed to deliver aggressive, punchy, JJP-style compression with harmonic enhancement and parallel processing capabilities. It combines modern digital precision with analog-inspired warmth.

---

## Signal Flow

```
Input Signal
    ↓
[1] Peak/RMS Detection (blendable)
    ↓
[2] Envelope Follower (Attack/Release)
    ↓
[3] Gain Reduction Calculation (Threshold, Ratio, Knee)
    ↓
    ├─→ [Metering] Track GR for visual feedback (needs debug)
    ↓
[4] Apply Compression to Signal
    ↓
[5] Tape Saturation (Drive)
    ↓
[6] Makeup Gain (Manual + Auto if enabled)
    ↓
[7] Parallel Mix (Dry/Wet blend)
    ↓
Output Signal
```

**New in this version:**
- **Auto Makeup Gain**: Automatically compensates for compression (80% of average GR)
- **Visual Metering**: Gain reduction meter with smooth ballistics (debugging in progress)

---

## Parameters

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

#### **Knee** (0 dB to 12 dB)
- **What it does**: Smooths the transition around the threshold
- **Default**: 3 dB
- **How it works**:
  - 0 dB = hard knee (instant compression at threshold)
  - Higher values = gradual compression onset
  - Uses quadratic interpolation for musical response
- **Sweet spots**:
  - **Punchy/Obvious**: 0-2 dB
  - **Balanced**: 3-6 dB (most musical)
  - **Smooth/Transparent**: 8-12 dB

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

#### **Drive** (0% to 100%)
- **What it does**: Adds tape-style harmonic saturation
- **Default**: 25%
- **Algorithm**: Soft tanh saturation with even harmonics for "sheen"
- **Sweet spots**:
  - **Clean (0-15%)**: Minimal saturation, pristine
  - **Subtle warmth (20-35%)**: Adds presence without distortion
  - **JJP shine (40-60%)**: Bright, aggressive, cuts through
  - **Heavy/Creative (70-100%)**: Obvious distortion, special effects

---

### Level & Blend

#### **Makeup Gain** (-20 dB to +50 dB)
- **What it does**: Compensates for volume loss from compression
- **Default**: 0 dB
- **How to set it**:
  - Match output level to input for A/B comparison
  - Typically add 3-10 dB for moderate compression
  - Higher compression needs more makeup gain

#### **Auto Makeup** (Toggle Switch)
- **What it does**: Automatically adds makeup gain based on compression amount
- **Default**: Off
- **How it works**:
  - Tracks average gain reduction over time (100ms smoothing)
  - Adds 80% of average GR as makeup (intentional under-compensation)
  - Works multiplicatively with manual makeup gain
- **Why 80% instead of 100%**:
  - Preserves some dynamic contrast (JJP-style punch)
  - Prevents over-leveling that can sound lifeless
  - Leaves room for manual fine-tuning
- **When to use it**:
  - **ON**: Quick workflow, parallel compression, "set and forget"
  - **OFF**: When you want precise manual control or extreme settings
- **JJP Secret**: Auto makeup at 80% + parallel mix at 50-70% = signature thick-but-punchy sound

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

**Note**: Meter display is currently being debugged and may show "-0.0 dB" even when compression is active. Functionality is implemented but needs troubleshooting.

---

## Preset Starting Points

### 1. JJP-Style Aggressive Vocal
**Goal**: Bright, punchy, upfront vocal that cuts through dense mixes

```
Threshold:    -35 dB
Ratio:        10:1
Attack:       2 ms
Release:      150 ms
Knee:         4 dB
Detection:    40% (peak-heavy)
Drive:        50%
Makeup Gain:  +5 dB
Auto Makeup:  ON
Mix:          60%
```

**Why it works**: Fast attack catches transients, heavy ratio controls dynamics, peak detection adds aggression, drive adds shine, parallel mix retains punch. Auto makeup compensates for ~8-12 dB of gain reduction.

**JJP Secret**: The combination of 50% drive + 60% parallel mix creates the signature "thick sheen" without sounding over-processed. Auto makeup keeps levels consistent when adjusting threshold.

---

### 2. Transparent Vocal Polish
**Goal**: Subtle control that doesn't sound compressed

```
Threshold:    -15 dB
Ratio:        3:1
Attack:       30 ms
Release:      300 ms
Knee:         8 dB
Detection:    80% (RMS-heavy)
Drive:        15%
Makeup Gain:  +4 dB
Mix:          100%
```

**Why it works**: Moderate threshold, gentle ratio, large knee for smooth transition, RMS detection for natural feel, minimal saturation.

---

### 3. Thick Parallel Compression
**Goal**: Add body and presence without changing dynamics

```
Threshold:    -30 dB
Ratio:        8:1
Attack:       5 ms
Release:      100 ms
Knee:         3 dB
Detection:    50%
Drive:        35%
Makeup Gain:  0 dB
Auto Makeup:  ON
Mix:          40%
```

**Why it works**: Heavy compression on wet signal blended subtly with dry creates thickness without losing transients. Auto makeup handles level compensation so you can focus on the mix blend.

**Pro Tip**: With auto makeup ON, you can freely adjust threshold and ratio without worrying about level changes. The 40% mix keeps the natural dynamics while adding controlled density.

---

### 4. Rap/Hip-Hop Vocal
**Goal**: Aggressive, in-your-face, heavily processed

```
Threshold:    -40 dB
Ratio:        15:1
Attack:       1 ms
Release:      80 ms
Knee:         2 dB
Detection:    20% (peak-dominant)
Drive:        60%
Makeup Gain:  +15 dB
Mix:          70%
```

**Why it works**: Very fast attack controls every syllable, high ratio for consistent level, peak detection for aggression, high drive for edge.

---

### 5. Ballad/Smooth Vocal
**Goal**: Even dynamics without harshness

```
Threshold:    -25 dB
Ratio:        4:1
Attack:       20 ms
Release:      800 ms
Knee:         10 dB
Detection:    90% (RMS)
Drive:        20%
Makeup Gain:  +6 dB
Mix:          100%
```

**Why it works**: Gentle compression with slow release smooths performance, large knee prevents obvious compression, minimal saturation keeps it clean.

---

## Usage Tips

### General Workflow

1. **Set Threshold & Ratio first**: Get the basic compression working
2. **Enable Auto Makeup**: Turn on for easier workflow (optional but recommended)
3. **Adjust Attack/Release**: Shape the compression response
4. **Tune Detection**: Find the right balance of punch vs. smoothness
5. **Add Drive**: Introduce harmonic enhancement
6. **Set Mix**: Decide between serial or parallel
7. **Fine-tune Makeup**: Adjust manual makeup if needed (auto handles most of it)
8. **Check the Meter**: Aim for 8-15 dB GR for JJP-style compression (when meter is working)

### Common Mistakes to Avoid

- **Too much compression + too much drive**: Creates harsh, pumping artifacts
- **Attack too fast with 100% mix**: Kills transients, sounds lifeless
- **Release too short**: Creates obvious pumping
- **Knee too small with high ratio**: Sounds unnatural and obvious

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
- Hard knee: Instant compression at threshold
- Soft knee: Quadratic interpolation within knee range
  - Formula: `gainReductionDb = (x²) / (2 * knee) * (1 - 1/ratio)`
- Above knee: Linear compression based on ratio

### Saturation Algorithm
- Soft tape saturation using hyperbolic tangent (tanh)
- Two-stage: Primary saturation + even harmonic enhancement
- Drive scales from 1x to 4x input gain
- Automatic gain compensation prevents level jumps

### Parallel Processing
- Dry signal preserved completely
- Compression → Saturation → Makeup applied to wet path
- Final mix: `output = (dry * (1 - mix)) + (wet * mix)`

---

## The JJP Signature Sound Formula

The VX1 achieves the classic JJP vocal sound through a specific combination of features:

### Core Recipe:
1. **Heavy Compression**: 8-15 dB of gain reduction (check meter when working)
2. **Parallel Mix**: 50-70% wet (retains transients while adding density)
3. **Auto Makeup at 80%**: Maintains punch by slightly under-compensating
4. **Drive at 40-60%**: Adds harmonic "sheen" and presence
5. **Peak Detection (20-50%)**: Adds aggression and attack

### Why It Works:
- **The 80% Auto Makeup**: Full compensation (100%) would make everything feel flat. The 20% "missing" level creates forward motion and energy.
- **Parallel + Under-compensation**: Together, they create the "thick but punchy" paradox - density without losing dynamics.
- **Drive + Parallel**: Heavy saturation on the wet signal only = character without distortion.

### Quick JJP Setup:
1. Set threshold for 10-12 dB gain reduction
2. Turn Auto Makeup ON
3. Set Mix to 60%
4. Set Drive to 50%
5. Adjust Detection to taste (40% is a good start)
6. Fine-tune with manual makeup if needed

---

## Parameter Interactions

### Threshold + Ratio
- Low threshold + low ratio = gentle leveling
- Low threshold + high ratio = heavy compression
- High threshold + high ratio = peak limiting only

### Attack + Detection
- Fast attack + Peak detection = aggressive transient control
- Slow attack + RMS detection = smooth, natural compression

### Drive + Mix
- High drive + low mix = adds character without over-processing
- Low drive + high mix = clean compression
- High drive + high mix = maximum aggression (use carefully!)

### Knee + Ratio
- Small knee + high ratio = obvious, grabby compression
- Large knee + high ratio = smooth compression that sneaks in
- Large knee + low ratio = very transparent

---

