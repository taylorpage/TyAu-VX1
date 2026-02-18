//
//  VX1ExtensionDSPKernel.hpp
//  VX1Extension
//
//  Compressor plugin for TyAu-VX1
//

#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <algorithm>
#include <span>
#include <cmath>
#include <vector>

#include "VX1ExtensionParameterAddresses.h"

/*
 VX1ExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.
 */
class VX1ExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
        mChannelCount = inputChannelCount;

        // Initialize computed coefficients
        mThresholdLinear = std::pow(10.0f, mThresholdDb / 20.0f);
        mMakeupGainLinear = std::pow(10.0f, mMakeupGainDb / 20.0f);
        mAttackCoeff = std::exp(-1.0f / (mAttackMs * 0.001f * mSampleRate));
        mReleaseCoeff = std::exp(-1.0f / (mReleaseMs * 0.001f * mSampleRate));
        // RMS detection: ~175ms squared-sample IIR window (averages across syllables, not individual transients)
        mRmsCoeff = std::exp(-1.0f / (0.175f * (float)mSampleRate));
        // Peak detection: ~2ms fast attack (aggressive on vocals without distortion artifacts)
        mInstantCoeff = std::exp(-1.0f / (0.002f * (float)mSampleRate));

        // GR overshoot timing (VCA punch): 0.5ms hold, 2ms exponential release
        mOvershootReleaseCoeff = std::exp(-1.0f / (0.002f * (float)mSampleRate));
        mOvershootHoldSamples  = static_cast<int>(0.0005f * mSampleRate);

        // Noise gate timing: 0.5ms attack, 100ms release, 50ms hold
        mGateAttackCoeff  = std::exp(-1.0f / (0.0005f * (float)mSampleRate));
        mGateReleaseCoeff = std::exp(-1.0f / (0.100f  * (float)mSampleRate));
        mGateHoldSamples  = static_cast<int>(0.050f   * mSampleRate);

        // Compute sidechain HPF coefficients for current sample rate
        computeHpfCoefficients();

        // Allocate per-channel presence shelf state and compute coefficients
        mPreX1.assign(inputChannelCount, 0.0f);
        mPreY1.assign(inputChannelCount, 0.0f);
        mDeX1.assign(inputChannelCount,  0.0f);
        mDeY1.assign(inputChannelCount,  0.0f);
        computePresenceCoefficients();

        // Reset state
        mEnvelopeLevel = 0.0f;
        mRmsState = 0.0f;
    }

    void deInitialize() {
        // Reset all state when deallocating
        mEnvelopeLevel = 0.0f;
        mCurrentGainReductionDb = 0.0f;

        // Reset RMS detection state
        mRmsState = 0.0f;

        // Reset sidechain HPF state
        mHpfX1 = mHpfX2 = mHpfY1 = mHpfY2 = 0.0f;

        // Reset sheen saturation presence filter state
        mPreX1.clear(); mPreY1.clear();
        mDeX1.clear();  mDeY1.clear();

        // Reset GR overshoot state
        mPrevGainReductionDb = 0.0f;
        mOvershootDb = 0.0f;
        mOvershootHoldCounter = 0;

        // Reset Stack second-pass state
        mEnvelopeLevel2 = 0.0f;
        mRmsState2 = 0.0f;
        mPrevGainReductionDb2 = 0.0f;
        mOvershootDb2 = 0.0f;
        mOvershootHoldCounter2 = 0;

        // Reset gate state
        mGateEnvelope = 0.0f;
        mGateGain = 1.0f;
        mGateHoldCounter = 0;
        mGateOpen = true;
    }

    // MARK: - Bypass
    bool isBypassed() {
        return mBypassed;
    }

    void setBypass(bool shouldBypass) {
        mBypassed = shouldBypass;
    }

    // MARK: - Parameter Getter / Setter
    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case VX1ExtensionParameterAddress::compress: {
                mCompressPercent = value;
                float t = mCompressPercent / 100.0f;          // 0.0 → 1.0 (linear knob position)
                float tThresh = std::pow(t, 0.2f);             // ^(1/5) curve: threshold drops extremely fast early
                mThresholdDb = tThresh * -50.0f;               // 0% → 0dB, 100% → -50dB
                mRatio = 1.0f + t * 29.0f;                    // 0% → 1:1, 100% → 30:1 (linear)
                mThresholdLinear = std::pow(10.0f, mThresholdDb / 20.0f);
                break;
            }
            case VX1ExtensionParameterAddress::speed:
                mSpeedMs = value;
                mAttackMs = mSpeedMs;
                mReleaseMs = mSpeedMs * 3.0f;
                mAttackCoeff = std::exp(-1.0f / (mAttackMs * 0.001f * mSampleRate));
                mReleaseCoeff = std::exp(-1.0f / (mReleaseMs * 0.001f * mSampleRate));
                break;
            case VX1ExtensionParameterAddress::makeupGain:
                mMakeupGainDb = value;
                mMakeupGainLinear = std::pow(10.0f, mMakeupGainDb / 20.0f);
                break;
            case VX1ExtensionParameterAddress::bypass:
                mBypassed = (value >= 0.5f);
                break;
            case VX1ExtensionParameterAddress::mix:
                mMixPercent = value;
                break;
            case VX1ExtensionParameterAddress::grip:
                mGripPercent = value;
                break;
            case VX1ExtensionParameterAddress::bite:
                mBitePercent = value;
                break;
            case VX1ExtensionParameterAddress::stack:
                mStackPercent = value;
                break;
            case VX1ExtensionParameterAddress::gateThreshold:
                mGateThresholdDb = value;
                break;
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        switch (address) {
            case VX1ExtensionParameterAddress::compress:
                return (AUValue)mCompressPercent;
            case VX1ExtensionParameterAddress::speed:
                return (AUValue)mSpeedMs;
            case VX1ExtensionParameterAddress::makeupGain:
                return (AUValue)mMakeupGainDb;
            case VX1ExtensionParameterAddress::bypass:
                return (AUValue)(mBypassed ? 1.0f : 0.0f);
            case VX1ExtensionParameterAddress::mix:
                return (AUValue)mMixPercent;
            case VX1ExtensionParameterAddress::grip:
                return (AUValue)mGripPercent;
            case VX1ExtensionParameterAddress::bite:
                return (AUValue)mBitePercent;
            case VX1ExtensionParameterAddress::stack:
                return (AUValue)mStackPercent;
            case VX1ExtensionParameterAddress::gainReductionMeter:
                return (AUValue)mCurrentGainReductionDb;
            case VX1ExtensionParameterAddress::gateThreshold:
                return (AUValue)mGateThresholdDb;
            default:
                return 0.f;
        }
    }

    // MARK: - Max Frames
    AUAudioFrameCount maximumFramesToRender() const {
        return mMaxFramesToRender;
    }

    void setMaximumFramesToRender(const AUAudioFrameCount &maxFrames) {
        mMaxFramesToRender = maxFrames;
    }

    // MARK: - Musical Context
    void setMusicalContextBlock(AUHostMusicalContextBlock contextBlock) {
        mMusicalContextBlock = contextBlock;
    }

    // MARK: - Sheen Saturation
    /**
     Four-stage "sheen" saturation — produces JJP-style aggressive presence shimmer.

     Signal flow:
       input
         │
         ├─[Stage 1a: Pre-emphasis] 1-pole +5 dB high shelf @ 3.5 kHz
         │   Biases wave shaper harmonic generation toward presence/air band.
         │   Interpolated by blend so Drive=0% is fully transparent.
         │
         ├─[Stage 2: Asymmetric wave shaper] tanh with small DC offset
         │   DC offset (scales with Sheen) makes positive half-cycles clip harder,
         │   generating stronger 2nd harmonic (octave) — the "sparkle/sheen" quality.
         │   DC removed post-shaping so output stays centered.
         │
         ├─[Stage 3: Cubic grit layer] x^3 component at low blend
         │   Adds 3rd harmonic (two octaves up, 4–12 kHz for vocal fundamentals).
         │   Provides "cuts through glass" edge without muddiness.
         │
         ├─[Stage 1b: De-emphasis] matching -5 dB shelf cut @ 3.5 kHz
         │   Restores tonal balance of the underlying signal. The newly generated
         │   harmonics are NOT cancelled — only the boosted fundamentals are restored.
         │   Net result: harmonic coloration weighted toward presence band.
         │
         └─[Stage 4: Gain compensation + dry/wet blend]
             Fixed formula keeps wet path at consistent loudness across all Sheen values.
             Previous algorithm under-compensated by ~10 dB at Sheen=100%.

     @param channel  Per-channel index needed for stateful pre/de-emphasis filters.
     */
    float applySaturation(float input, float amount, int channel) {
        if (amount <= 0.0f) return input;

        const float blend = amount / 100.0f;

        // --- Stage 1a: Pre-emphasis high shelf (+5 dB @ 3.5 kHz) ---
        // Harmonic generation is louder above 3.5 kHz → presence-band sheen
        float preOut = mShelfB0Pre * input
                     + mShelfB1Pre * mPreX1[channel]
                     - mShelfA1Pre * mPreY1[channel];
        mPreX1[channel] = input;
        mPreY1[channel] = preOut;
        // Scale shelf in with Sheen amount: transparent at 0%, full boost at 100%
        float emphasized = input + (preOut - input) * blend;

        // --- Stage 2: Asymmetric wave shaper (2nd harmonic — "sheen/sparkle") ---
        // Small positive DC offset makes the wave shaper clip asymmetrically,
        // generating stronger even harmonics (2nd harmonic = octave above fundamental).
        float drive     = 1.0f + blend * 4.0f;    // 1.0 at 0% → 5.0 at 100%
        float dcOffset  = 0.18f * blend;           // offset grows with Bite amount
        float driven    = (emphasized + dcOffset) * drive * 1.3f;
        float shaped    = std::tanh(driven);
        float shapedDc  = std::tanh(dcOffset * drive * 1.3f);
        shaped -= shapedDc;                        // remove DC from asymmetry

        // --- Stage 3: Cubic grit layer (3rd harmonic — "edge") ---
        // x^3 generates 3rd harmonic (two octaves up), sits in 4–12 kHz for vocals.
        // Scaled by (1 - blend*0.5) so it fades back at high drive where shaped is
        // already near clipping — prevents aliasing/intermodulation at top of knob.
        float cubic    = shaped * shaped * shaped;
        float gritAmt  = 0.06f * blend * (1.0f - blend * 0.5f);
        float withGrit = shaped + cubic * gritAmt;

        // --- Stage 1b: De-emphasis high shelf (-5 dB @ 3.5 kHz) ---
        // Restores the tonal balance of the fundamental content.
        // Generated harmonics live above the shelf region so they survive.
        float deOut = mShelfB0De * withGrit
                    + mShelfB1De * mDeX1[channel]
                    - mShelfA1De * mDeY1[channel];
        mDeX1[channel] = withGrit;
        mDeY1[channel] = deOut;
        float deEmphasized = withGrit + (deOut - withGrit) * blend;

        // --- Stage 4: Gain compensation ---
        // Normalize the wet path back to unity by inverting the tanh's actual ceiling
        // at the current drive setting. 1/tanh(drive*1.3) exactly compensates for
        // how much the wave shaper has compressed the signal, keeping perceived level
        // consistent across the full knob range.
        float compensationGain = 1.0f / std::tanh(drive * 1.3f);
        deEmphasized *= compensationGain;

        // Final dry/wet blend
        return input * (1.0f - blend) + deEmphasized * blend;
    }

    // MARK: - Sheen Saturation: Presence Pre/De-Emphasis

    /**
     Computes 1-pole high-shelf coefficients for the sheen saturation stage.
     Cutoff: ~3.5 kHz, Gain: +5 dB (pre-emphasis) / -5 dB (de-emphasis).

     By boosting the high frequencies BEFORE the wave shaper and cutting AFTER,
     harmonic distortion is generated predominantly in the 3.5–8 kHz presence band
     rather than the low-mid. This mimics the transformer coloration of hardware
     like the Neve 1073 and SSL 4000 channel — the source of the JJP "sheen".

     Bilinear-transform 1-pole shelf design:
       K = tan(π * fc / fs)
       Boost:  b0 = (G*K + 1)/(K + 1),  b1 = (G*K - 1)/(K + 1),  a1 = (K - 1)/(K + 1)
       Cut:    b0 = (K + 1)/(G*K + 1),  b1 = (K - 1)/(G*K + 1),  a1 = (G*K - 1)/(G*K + 1)
       where G = linear gain = 10^(5/20) ≈ 1.778
     */
    void computePresenceCoefficients() {
        const float fc = 3500.0f;
        const float gainDb = 5.0f;
        const float G = std::pow(10.0f, gainDb / 20.0f);  // ≈ 1.778
        const float K = std::tan((float)M_PI * fc / (float)mSampleRate);

        // Pre-emphasis: +5 dB shelf boost above 3.5 kHz
        mShelfB0Pre = (G * K + 1.0f) / (K + 1.0f);
        mShelfB1Pre = (G * K - 1.0f) / (K + 1.0f);
        mShelfA1Pre = (K - 1.0f)     / (K + 1.0f);

        // De-emphasis: matching -5 dB shelf cut (exact inverse)
        mShelfB0De  = (K + 1.0f)     / (G * K + 1.0f);
        mShelfB1De  = (K - 1.0f)     / (G * K + 1.0f);
        mShelfA1De  = (G * K - 1.0f) / (G * K + 1.0f);
    }

    // MARK: - Sidechain HPF

    /// Computes 2-pole Butterworth HPF coefficients for the fixed 80 Hz sidechain filter.
    /// Must be called once per initialize() and whenever sample rate changes.
    void computeHpfCoefficients() {
        const float fc = 80.0f;
        const float omega = 2.0f * (float)M_PI * fc / (float)mSampleRate;
        const float cosOmega = std::cos(omega);
        const float sinOmega = std::sin(omega);
        // Butterworth: Q = 1/sqrt(2) ≈ 0.7071
        const float alpha = sinOmega / (2.0f * 0.7071f);

        float b0 =  (1.0f + cosOmega) / 2.0f;
        float b1 = -(1.0f + cosOmega);
        float b2 =  (1.0f + cosOmega) / 2.0f;
        float a0 =   1.0f + alpha;
        float a1 =  -2.0f * cosOmega;
        float a2 =   1.0f - alpha;

        mHpfA0 = b0 / a0;
        mHpfA1 = b1 / a0;
        mHpfA2 = b2 / a0;
        mHpfB1 = a1 / a0;
        mHpfB2 = a2 / a0;
    }

    /// Runs one sample through the sidechain HPF (Direct Form II Transposed).
    float applyHpf(float x) {
        float y = mHpfA0 * x + mHpfA1 * mHpfX1 + mHpfA2 * mHpfX2
                             - mHpfB1 * mHpfY1  - mHpfB2 * mHpfY2;
        mHpfX2 = mHpfX1; mHpfX1 = x;
        mHpfY2 = mHpfY1; mHpfY1 = y;
        return y;
    }

    /**
     MARK: - Internal Process

     This function does the core signal processing.
     Implements a feed-forward RMS compressor with attack/release envelope follower.
     */
    void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers, AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {
        assert(inputBuffers.size() == outputBuffers.size());

        if (mBypassed) {
            // Pass the samples through unmodified
            for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                std::copy_n(inputBuffers[channel], frameCount, outputBuffers[channel]);
            }
            mCurrentGainReductionDb = 0.0f;
        } else {
            // Track peak gain reduction in this buffer
            float peakGainReductionDb = 0.0f;

            // Process each frame
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {

                // --- Noise Gate: pre-input-gain, runs on raw input level ---
                // Envelope follower on the peak of the raw (pre-gain) mono sum.
                // When signal drops below threshold: hold for 50ms, then close over 100ms.
                // Gate gain (0=closed, 1=open) is applied to both sidechain and audio paths.
                {
                    float rawMono = 0.0f;
                    for (UInt32 ch = 0; ch < inputBuffers.size(); ++ch) {
                        rawMono += std::abs(inputBuffers[ch][frameIndex]);
                    }
                    rawMono /= (float)inputBuffers.size();

                    // Peak envelope follower: fast attack, slow release
                    if (rawMono > mGateEnvelope) {
                        mGateEnvelope = mGateAttackCoeff * mGateEnvelope + (1.0f - mGateAttackCoeff) * rawMono;
                    } else {
                        mGateEnvelope = mGateReleaseCoeff * mGateEnvelope + (1.0f - mGateReleaseCoeff) * rawMono;
                    }

                    float gateThresholdLinear = std::pow(10.0f, mGateThresholdDb / 20.0f);
                    bool signalAboveThreshold = (mGateEnvelope >= gateThresholdLinear);

                    if (signalAboveThreshold) {
                        // Signal present: open gate, reset hold counter
                        mGateOpen = true;
                        mGateHoldCounter = mGateHoldSamples;
                        mGateGain = 1.0f;  // snap open instantly
                    } else if (mGateHoldCounter > 0) {
                        // Signal gone but still in hold period: stay open
                        mGateHoldCounter--;
                        mGateGain = 1.0f;
                    } else {
                        // Hold expired: close gate with smoothed release
                        mGateOpen = false;
                        mGateGain *= mGateReleaseCoeff;
                    }
                }

                // --- Detection: always runs on the current (undelayed) input ---
                // Sidechain signal: mono sum → fixed 80 Hz HPF
                float monoSC = 0.0f;
                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    monoSC += inputBuffers[channel][frameIndex] * mGateGain;
                }
                monoSC /= (float)inputBuffers.size();
                float filteredSC = applyHpf(monoSC);
                float absFiltered = std::abs(filteredSC);

                // Peak detection: instantaneous absolute value — grabs transients hard
                float peak = absFiltered;

                // RMS detection: IIR squared-sample accumulator (~50ms window)
                // Responds to energy, not individual peaks — smooth and musical
                mRmsState = mRmsCoeff * mRmsState + (1.0f - mRmsCoeff) * (absFiltered * absFiltered);
                float rms = std::sqrt(mRmsState);

                // Blend detected level: 0% = pure RMS (smooth), 100% = pure Peak (tight/aggressive)
                float gripBlend = mGripPercent / 100.0f;
                float detectionLevel = (rms * (1.0f - gripBlend)) + (peak * gripBlend);

                // Dramatic mode difference: envelope attack changes with grip knob
                // RMS (0%): uses the user's attack knob — compressor breathes with the music
                // Peak (100%): ~2ms near-instant attack — compressor slams on every transient
                float blendedAttackCoeff = mAttackCoeff * (1.0f - gripBlend) + mInstantCoeff * gripBlend;

                // Envelope follower (blended attack, fixed release)
                float coeff = (detectionLevel > mEnvelopeLevel) ? blendedAttackCoeff : mReleaseCoeff;
                mEnvelopeLevel = coeff * mEnvelopeLevel + (1.0f - coeff) * detectionLevel;

                // Calculate gain reduction (hard knee)
                float gainReduction = 1.0f;
                float gainReductionDb = 0.0f;
                float envelopeDb = 20.0f * std::log10(std::max(mEnvelopeLevel, 1e-6f));
                float overThresholdDb = envelopeDb - mThresholdDb;

                if (overThresholdDb > 0.0f) {
                    gainReductionDb = overThresholdDb * (1.0f - 1.0f / mRatio);
                    gainReduction = std::pow(10.0f, -gainReductionDb / 20.0f);
                }
                // else: below threshold, gainReduction stays at 1.0

                // --- GR Overshoot: VCA-style transient punch ---
                // Replicates the physical overshoot of a VCA gain cell (dbx 160 / SSL G-bus):
                // when a transient causes GR to jump by more than 3 dB in one sample,
                // briefly over-apply 3 dB of extra GR for 0.5ms (hold), then release
                // exponentially over 2ms. Creates the "slammed" transient grab feel.
                float grJump = gainReductionDb - mPrevGainReductionDb;
                if (grJump > 3.0f) {
                    mOvershootDb = 3.0f;
                    mOvershootHoldCounter = mOvershootHoldSamples;
                }
                mPrevGainReductionDb = gainReductionDb;
                if (mOvershootHoldCounter > 0) {
                    mOvershootHoldCounter--;              // hold phase: overshoot stays fixed
                } else {
                    mOvershootDb *= mOvershootReleaseCoeff; // release phase: exponential decay
                }
                float totalGainReductionDb = gainReductionDb + mOvershootDb;
                float gainReductionTotal = std::pow(10.0f, -totalGainReductionDb / 20.0f);

                // Track peak gain reduction for metering (includes overshoot — meter shows what you hear)
                peakGainReductionDb = std::max(peakGainReductionDb, totalGainReductionDb);

                // --- Stack: true serial second compression pass ---
                // Detects on the post-pass-1 signal so the second stage sees an already-compressed
                // input, just like chaining two hardware units. Stack lowers the second stage's
                // threshold (making it hit progressively harder as the knob increases).
                // GR multiplies — no blend/lerp — so at any Stack > 0 you feel the stacking.
                float stackBlend = mStackPercent / 100.0f;
                float gainReductionTotal2 = 1.0f;

                if (stackBlend > 0.0f) {
                    // Sidechain: mono sum of post-pass-1 audio
                    float monoPost1 = 0.0f;
                    for (UInt32 ch = 0; ch < inputBuffers.size(); ++ch) {
                        monoPost1 += inputBuffers[ch][frameIndex] * mGateGain * gainReductionTotal;
                    }
                    monoPost1 /= (float)inputBuffers.size();
                    float absPost1 = std::abs(monoPost1);

                    float peak2 = absPost1;
                    mRmsState2 = mRmsCoeff * mRmsState2 + (1.0f - mRmsCoeff) * (absPost1 * absPost1);
                    float rms2 = std::sqrt(mRmsState2);

                    float detectionLevel2 = (rms2 * (1.0f - gripBlend)) + (peak2 * gripBlend);
                    float coeff2 = (detectionLevel2 > mEnvelopeLevel2) ? blendedAttackCoeff : mReleaseCoeff;
                    mEnvelopeLevel2 = coeff2 * mEnvelopeLevel2 + (1.0f - coeff2) * detectionLevel2;

                    // Stack knob lowers the second stage threshold proportionally so it bites
                    // harder as you turn it up. At 100% Stack the threshold is halved in dB.
                    float thresholdDb2 = mThresholdDb + (mThresholdDb * stackBlend * 0.5f);

                    float gainReductionDb2 = 0.0f;
                    float envelopeDb2 = 20.0f * std::log10(std::max(mEnvelopeLevel2, 1e-6f));
                    float overThresholdDb2 = envelopeDb2 - thresholdDb2;
                    if (overThresholdDb2 > 0.0f) {
                        gainReductionDb2 = overThresholdDb2 * (1.0f - 1.0f / mRatio);
                        gainReductionTotal2 = std::pow(10.0f, -gainReductionDb2 / 20.0f);
                    }

                    // VCA overshoot on pass 2
                    float grJump2 = gainReductionDb2 - mPrevGainReductionDb2;
                    if (grJump2 > 3.0f) {
                        mOvershootDb2 = 3.0f;
                        mOvershootHoldCounter2 = mOvershootHoldSamples;
                    }
                    mPrevGainReductionDb2 = gainReductionDb2;
                    if (mOvershootHoldCounter2 > 0) {
                        mOvershootHoldCounter2--;
                    } else {
                        mOvershootDb2 *= mOvershootReleaseCoeff;
                    }
                    float totalGainReductionDb2 = gainReductionDb2 + mOvershootDb2;
                    gainReductionTotal2 = std::pow(10.0f, -totalGainReductionDb2 / 20.0f);

                    peakGainReductionDb = std::max(peakGainReductionDb,
                                                   totalGainReductionDb + totalGainReductionDb2);
                }

                // Stack auto-makeup: compensate for the expected additional GR from pass 2.
                // Derived from how much the second stage's threshold was lowered:
                //   extraThresholdDb = mThresholdDb * stackBlend * 0.5  (negative number)
                //   expectedGR2Db    = -extraThresholdDb * (1 - 1/ratio) (positive dB)
                // This is static per Stack value (not per-sample), so it is stable and
                // does not add pumping. At Stack=0 it evaluates to exactly 1.0 (no change).
                float stackMakeupGain = 1.0f;
                if (stackBlend > 0.0f) {
                    float extraThresholdDb  = mThresholdDb * stackBlend * 0.5f;   // e.g. -5 dB at 50%
                    float expectedGR2Db     = -extraThresholdDb * (1.0f - 1.0f / mRatio);
                    stackMakeupGain = std::pow(10.0f, expectedGR2Db / 20.0f);
                }

                // Apply compression, saturation, makeup gain, then mix with dry signal
                float mixWet = mMixPercent / 100.0f;
                float mixDry = 1.0f - mixWet;

                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    float audioInput = inputBuffers[channel][frameIndex] * mGateGain;

                    // Apply pass 1 compression, then pass 2 GR multiplies on top (true serial stacking).
                    // stackMakeupGain compensates for the expected volume drop from the second pass.
                    float compressed = audioInput * gainReductionTotal * gainReductionTotal2 * stackMakeupGain;

                    // Apply sheen saturation (presence-biased harmonic coloration)
                    float saturated = applySaturation(compressed, mBitePercent, (int)channel);

                    // Apply makeup gain
                    saturated *= mMakeupGainLinear;

                    // Parallel mix: blend dry and processed signals
                    float output = (audioInput * mixDry) + (saturated * mixWet);
                    outputBuffers[channel][frameIndex] = output;
                }

            }

            // Update meter with peak gain reduction from this buffer
            // Apply smoothing for visual stability (instant attack, adaptive release)
            if (peakGainReductionDb > mCurrentGainReductionDb) {
                // Attack - snap immediately to peak so the needle reacts without lag
                mCurrentGainReductionDb = peakGainReductionDb;
            } else {
                // Release - use adaptive strategy based on how close to zero we are
                if (peakGainReductionDb < 0.05f) {
                    // When minimal or no compression, snap to zero immediately
                    // This ensures meter resets quickly when audio stops
                    mCurrentGainReductionDb = 0.0f;
                } else if (peakGainReductionDb < 1.0f) {
                    // Fast release when light compression (0.5 coefficient = much faster)
                    mCurrentGainReductionDb = 0.5f * mCurrentGainReductionDb + 0.5f * peakGainReductionDb;
                } else {
                    // Normal slow release for readability during active compression
                    float meterReleaseCoeff = 0.95f;
                    mCurrentGainReductionDb = meterReleaseCoeff * mCurrentGainReductionDb + (1.0f - meterReleaseCoeff) * peakGainReductionDb;
                }
            }

        }
    }

    void handleOneEvent(AUEventSampleTime now, AURenderEvent const *event) {
        switch (event->head.eventType) {
            case AURenderEventParameter: {
                handleParameterEvent(now, event->parameter);
                break;
            }
            default:
                break;
        }
    }

    void handleParameterEvent(AUEventSampleTime now, AUParameterEvent const& parameterEvent) {
        setParameter(parameterEvent.parameterAddress, parameterEvent.value);
    }

    // MARK: Member Variables
    AUHostMusicalContextBlock mMusicalContextBlock;
    double mSampleRate = 44100.0;
    bool mBypassed = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;

    // Compressor parameters (in dB and ms)
    float mCompressPercent = 30.0f; // 0% = no compression (0dB thresh, 1:1), 100% = max (−50dB thresh, 30:1)
    float mThresholdDb = -15.0f;    // Derived from mCompressPercent
    float mRatio = 9.7f;            // Derived from mCompressPercent
    float mSpeedMs = 10.0f;   // Speed knob; attack = mSpeedMs, release = mSpeedMs * 3
    float mAttackMs = 10.0f;  // Derived: mSpeedMs
    float mReleaseMs = 30.0f; // Derived: mSpeedMs * 3
    float mMakeupGainDb = 0.0f;
    float mMixPercent = 100.0f;
    float mGripPercent = 0.0f;    // 0% = RMS (smooth), 100% = Peak (tight/aggressive)
    float mBitePercent = 25.0f;   // 0% = Clean, 100% = Aggressive presence-biased harmonic bite
    float mStackPercent = 0.0f;   // 0% = single compression pass, 100% = double compression pass

    // Computed/cached values (linear)
    float mThresholdLinear = 0.1f;  // 10^(thresholdDb/20)
    float mMakeupGainLinear = 1.0f; // 10^(makeupGainDb/20)
    float mAttackCoeff = 0.0f;      // exp(-1/(attackMs * 0.001 * sampleRate))
    float mReleaseCoeff = 0.0f;     // exp(-1/(releaseMs * 0.001 * sampleRate))
    float mRmsCoeff = 0.0f;         // exp(-1/(0.175 * sampleRate)) — ~175ms RMS window (vocal syllable averaging)
    float mInstantCoeff = 0.0f;     // exp(-1/(0.002 * sampleRate)) — ~2ms peak grab (fast but distortion-safe)

    // State
    float mEnvelopeLevel = 0.0f;           // Envelope follower state
    float mRmsState = 0.0f;                // IIR squared-sample accumulator for RMS detection
    float mCurrentGainReductionDb = 0.0f;  // Current gain reduction for metering

    // Stack — second-pass envelope follower state (independent from pass 1)
    float mEnvelopeLevel2 = 0.0f;
    float mRmsState2 = 0.0f;
    float mPrevGainReductionDb2 = 0.0f;
    float mOvershootDb2 = 0.0f;
    int   mOvershootHoldCounter2 = 0;

    // Channel count
    int mChannelCount = 2;                 // Set during initialize()

    // Sidechain HPF — fixed 80 Hz 2-pole Butterworth, detection path only
    float mHpfX1 = 0.0f, mHpfX2 = 0.0f;  // input delay history
    float mHpfY1 = 0.0f, mHpfY2 = 0.0f;  // output delay history
    float mHpfA0 = 1.0f, mHpfA1 = -2.0f, mHpfA2 = 1.0f; // numerator coefficients
    float mHpfB1 = 0.0f, mHpfB2 = 0.0f;  // denominator coefficients (B0 normalised to 1)

    // Sheen saturation — presence pre/de-emphasis filter state (per channel)
    // 1-pole high shelf at ~3.5 kHz: boosts before saturation, cuts after
    // Result: harmonic generation is biased toward presence/air band (Neve/SSL transformer character)
    std::vector<float> mPreX1, mPreY1;   // pre-emphasis shelf: input and output history per channel
    std::vector<float> mDeX1,  mDeY1;    // de-emphasis shelf:  input and output history per channel
    float mShelfB0Pre = 1.0f, mShelfB1Pre = 0.0f, mShelfA1Pre = 0.0f; // pre-emphasis coefficients
    float mShelfB0De  = 1.0f, mShelfB1De  = 0.0f, mShelfA1De  = 0.0f; // de-emphasis coefficients

    // GR overshoot — VCA-style transient punch
    // When a transient causes GR to jump >3 dB in one sample, over-apply 3 dB extra GR
    // for a brief hold (0.5ms), then exponentially release back over 2ms.
    // Replicates the physical VCA overshoot of the dbx 160 / SSL G-bus gain cell.
    float mPrevGainReductionDb = 0.0f;   // GR from previous sample (for jump detection)
    float mOvershootDb = 0.0f;           // Currently active overshoot amount (decays to 0)
    float mOvershootReleaseCoeff = 0.0f; // exp(-1 / (2ms * sr)) — computed in initialize()
    int   mOvershootHoldSamples = 0;     // 0.5ms * sr — computed in initialize()
    int   mOvershootHoldCounter = 0;     // Counts down from mOvershootHoldSamples

    // Noise gate — pre-input-gain, before entire compressor chain
    // Threshold: -80 to -20 dB. At -80 dB (default) the gate is effectively always open.
    // Attack: 0.5ms (fast open), Hold: 50ms (prevents chatter), Release: 100ms (smooth close).
    float mGateThresholdDb = -80.0f;     // User-set threshold (-80 = off)
    float mGateEnvelope = 0.0f;          // Peak envelope follower on raw input (pre-gain)
    float mGateGain = 1.0f;              // Current gate gain scalar (0=closed, 1=open), smoothed
    float mGateAttackCoeff = 0.0f;       // exp(-1 / (0.5ms * sr))
    float mGateReleaseCoeff = 0.0f;      // exp(-1 / (100ms * sr))
    int   mGateHoldSamples = 0;          // 50ms * sr
    int   mGateHoldCounter = 0;          // Counts down when signal drops below threshold
    bool  mGateOpen = true;              // Current gate state (open/closed)
};
