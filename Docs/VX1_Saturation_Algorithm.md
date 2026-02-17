# VX1 Sheen Saturation — Algorithm Reference

## Overview

The **Sheen** control applies a four-stage harmonic saturation algorithm designed to
reproduce the "aggressive presence shimmer" characteristic of JJP vocal processing.
Unlike generic tape saturation (which generates harmonics uniformly across all
frequencies), this algorithm biases harmonic generation toward the **3.5–8 kHz
presence/air band** — the frequency range that makes vocals cut through a dense mix.

The algorithm is entirely controlled by the single Sheen knob (0–100%). At 0% it is
completely transparent. All character builds from there.

---

## Signal Flow

```
Input
  │
  ├─[Stage 1a] Pre-emphasis shelf (+5 dB high shelf @ 3.5 kHz)
  │              Blend: 0% at Sheen=0, full +5 dB at Sheen=100
  │              → Emphasizes presence band going INTO the wave shaper
  │
  ├─[Stage 2]  Asymmetric wave shaper (tanh + DC offset)
  │              DC offset = 0.08 × blend (positive, scales with Sheen)
  │              → Clips positive half-cycles harder → stronger 2nd harmonic (sheen/sparkle)
  │              DC removed post-shaping to keep signal centered
  │
  ├─[Stage 3]  Cubic grit layer (x³ component at 6% × blend)
  │              → Adds 3rd harmonic (edge/presence) — 4–12 kHz for vocal fundamentals
  │              Kept subtle to add "cuts through glass" quality without harshness
  │
  ├─[Stage 1b] De-emphasis shelf (−5 dB high shelf @ 3.5 kHz, exact inverse of 1a)
  │              → Restores tonal balance of the fundamental content
  │              Generated harmonics live above the shelf corner and are NOT cancelled
  │              Net result: coloration concentrated in presence band
  │
  └─[Stage 4]  Gain compensation + dry/wet blend
                 compensationGain = 1 / (0.5 + 0.5 × blend)
                 → Corrects tanh-induced level reduction
                 → Final mix: dry × (1 − blend) + wet × blend
```

---

## Stage-by-Stage Detail

### Stage 1: Pre-Emphasis / De-Emphasis (Presence Shelf)

**What it is**: A matched pair of 1-pole high-shelf filters — one boost before the wave
shaper, one cut of equal magnitude after.

**Why it works**: The wave shaper generates harmonics proportional to the *energy* of
the signal entering it. By boosting 3.5 kHz+ before shaping, we force it to generate
more harmonics in the presence/air band. The subsequent de-emphasis cut restores the
tonal balance of the original signal content, but the *newly generated harmonics*
(which sit above the fundamentals) survive.

This is the DSP equivalent of recording through a Neve 1073 or SSL 4000 channel strip —
both of which have transformer and op-amp characteristics that produce exactly this kind
of presence-weighted coloration.

**Filter design**: Bilinear-transform 1-pole shelving filter (Zölzer design):
```
fc = 3500 Hz
G  = 10^(5/20) ≈ 1.778   (linear gain for +5 dB)
K  = tan(π × fc / fs)     (bilinear transform pre-warping)

Pre-emphasis (boost):
  b0 = (G×K + 1) / (K + 1)
  b1 = (G×K − 1) / (K + 1)
  a1 =    (K − 1) / (K + 1)

De-emphasis (exact inverse):
  b0 =    (K + 1) / (G×K + 1)
  b1 =    (K − 1) / (G×K + 1)
  a1 = (G×K − 1) / (G×K + 1)

Difference equation: y[n] = b0×x[n] + b1×x[n-1] − a1×y[n-1]
```

Coefficients are recomputed in `initialize()` whenever the sample rate changes, so
the filter is accurate at 44.1, 48, and 96 kHz.

---

### Stage 2: Asymmetric Wave Shaper

**What it is**: A `tanh` soft clipper with a small positive DC offset applied before
shaping, then removed after.

**Why asymmetry matters**: A perfectly symmetric wave shaper (like plain `tanh(x)`)
produces only *odd* harmonics (3rd, 5th, 7th…). These give warmth and grit but not
sparkle. *Even* harmonics (2nd, 4th) — which are what transformers and tubes produce —
give that shimmer/sheen quality. Adding a DC offset before the wave shaper breaks the
symmetry and causes it to generate even harmonics.

```
drive      = 1.0 + blend × 3.0        (1.0 → 4.0 as Sheen: 0 → 100%)
dcOffset   = 0.08 × blend             (0 → 0.08 as Sheen: 0 → 100%)
driven     = (emphasized + dcOffset) × drive × 1.3
shaped     = tanh(driven)
shapedDc   = tanh(dcOffset × drive × 1.3)   ← what the DC offset shaped to
shaped    -= shapedDc                 ← remove the DC, keep the asymmetry
```

**Removing the DC offset**: If we don't subtract `shapedDc`, the output has a DC bias
that would accumulate through the envelope follower and cause issues. Subtracting the
shaped DC restores centering while preserving the asymmetric harmonic content.

---

### Stage 3: Cubic Grit Layer

**What it is**: A small addition of `x³` (the signal cubed) to the shaped signal.

**Why cubing**: `x³` is a mathematical 3rd-harmonic generator. For a vocal fundamental
at 300 Hz, the 3rd harmonic lands at 900 Hz. For a 1 kHz consonant, the 3rd harmonic
lands at 3 kHz — right in the presence band. For harmonics already in the 2–4 kHz
range, the cubic layer pushes energy up into 6–12 kHz (air/sheen territory).

```
cubic    = shaped³
withGrit = shaped + cubic × 0.06 × blend
```

The `0.06` coefficient keeps this subtle — just enough "edge" to add the sense of
aggression without introducing harshness or distortion artifacts.

---

### Stage 4: Gain Compensation

**The problem with the old algorithm**: The previous code used `(1/drive) × 1.2` as
its compensation factor. At Sheen=100% (drive=4.0), this gives `×0.30` — the wet
signal was approximately **−10 dB** relative to the input before the dry/wet blend.
This effectively buried the saturation character.

**The fix**: A formula that keeps the wet path near unity loudness at all Sheen values:
```
compensationGain = 1 / (0.5 + 0.5 × blend)
```
- Sheen=0%:   compensationGain = 1 / 0.50 = 2.0  (but blend=0 so dry only anyway)
- Sheen=25%:  compensationGain = 1 / 0.625 = 1.6
- Sheen=50%:  compensationGain = 1 / 0.75 = 1.33
- Sheen=100%: compensationGain = 1 / 1.00 = 1.0

This keeps the wet signal audible at all settings, so the Sheen character actually
reaches the output and can be heard in the parallel blend.

---

## Why This Sounds Like JJP

The classic JJP vocal sound has several DSP-identifiable characteristics:

1. **Presence-weighted coloration** — harmonics concentrated in 3–8 kHz, not the
   low-mid. This is exactly what the pre/de-emphasis shelf pair achieves.

2. **Even harmonic shimmer** — the 2nd harmonic "octave sheen" that tubes and
   transformers produce. The asymmetric DC offset in Stage 2 creates this directly.

3. **Controlled grit** — a subtle upper-harmonic "edge" that makes consonants cut
   through. The Stage 3 cubic layer provides this.

4. **Character that holds at high drive** — the original algorithm was too quiet
   at high settings, making the character disappear. Stage 4 ensures it doesn't.

The combination of all four stages, controlled by a single knob, achieves the
"thick but shiny" paradox: harmonic density without low-mid muddiness.

---

## Implementation Location

- **Algorithm**: `VX1Extension/DSP/VX1ExtensionDSPKernel.hpp`
  - `applySaturation(float input, float amount, int channel)` — main algorithm
  - `computePresenceCoefficients()` — computes shelf filter coefficients
- **Member variables**: `mPreX1/Y1`, `mDeX1/Y1` (per-channel filter state), `mShelfB0/B1/A1Pre/De`
- **Parameter address**: `sheen = 9` in `VX1ExtensionParameterAddresses.h`
- **UI**: "SHEEN" knob in fourth row of `VX1ExtensionMainView.swift`

---

## Sweet Spots

| Sheen % | Character |
|---------|-----------|
| 0%      | Completely transparent |
| 15–25%  | Subtle presence lift — adds forward quality without obvious distortion |
| 35–50%  | JJP-style "sheen" — vocal cuts through the mix, harmonics clearly audible |
| 50–65%  | Aggressive shine — very forward, works well with parallel mix at 50–70% |
| 70–100% | Heavy coloration — intentional effect, special uses |

**JJP Quick Start**: Sheen=50%, Mix=60%, Auto Makeup ON → the three controls that
recreate the signature JJP vocal character fastest.
