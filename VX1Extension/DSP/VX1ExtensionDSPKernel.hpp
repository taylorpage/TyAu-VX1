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
        mInputGainLinear  = std::pow(10.0f, mInputGainDb  / 20.0f);
        mAttackCoeff = std::exp(-1.0f / (mAttackMs * 0.001f * mSampleRate));
        mReleaseCoeff = std::exp(-1.0f / (mReleaseMs * 0.001f * mSampleRate));

        // GR overshoot timing (VCA punch): 0.5ms hold, 2ms exponential release
        mOvershootReleaseCoeff = std::exp(-1.0f / (0.002f * (float)mSampleRate));
        mOvershootHoldSamples  = static_cast<int>(0.0005f * mSampleRate);

        // Allocate look-ahead ring buffer: max 10ms at current sample rate, per channel
        // Always allocate max capacity so we never reallocate mid-stream
        int maxLookAheadSamples = static_cast<int>(std::ceil(0.010 * mSampleRate));
        mRingBuffer.assign(inputChannelCount, std::vector<float>(maxLookAheadSamples, 0.0f));
        mRingWritePos = 0;
        mLookAheadSamples = lookAheadIndexToSamples(mLookAheadIndex);

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
    }

    void deInitialize() {
        // Reset all state when deallocating
        mEnvelopeLevel = 0.0f;
        mAvgGainReductionDb = 0.0f;
        mCurrentGainReductionDb = 0.0f;
        mRingBuffer.clear();
        mRingWritePos = 0;

        // Reset sidechain HPF state
        mHpfX1 = mHpfX2 = mHpfY1 = mHpfY2 = 0.0f;

        // Reset sheen saturation presence filter state
        mPreX1.clear(); mPreY1.clear();
        mDeX1.clear();  mDeY1.clear();

        // Reset GR overshoot state
        mPrevGainReductionDb = 0.0f;
        mOvershootDb = 0.0f;
        mOvershootHoldCounter = 0;
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
            case VX1ExtensionParameterAddress::threshold:
                mThresholdDb = value;
                mThresholdLinear = std::pow(10.0f, mThresholdDb / 20.0f);
                break;
            case VX1ExtensionParameterAddress::ratio:
                mRatio = value;
                break;
            case VX1ExtensionParameterAddress::attack:
                mAttackMs = value;
                mAttackCoeff = std::exp(-1.0f / (mAttackMs * 0.001f * mSampleRate));
                break;
            case VX1ExtensionParameterAddress::release:
                mReleaseMs = value;
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
            case VX1ExtensionParameterAddress::knee:
                mKneeDb = value;
                break;
            case VX1ExtensionParameterAddress::detection:
                mDetectionPercent = value;
                break;
            case VX1ExtensionParameterAddress::sheen:
                mSheenPercent = value;
                break;
            case VX1ExtensionParameterAddress::autoMakeup:
                mAutoMakeupEnabled = (value >= 0.5f);
                break;
            case VX1ExtensionParameterAddress::lookAhead:
                mLookAheadIndex = static_cast<int>(value + 0.5f);
                mLookAheadSamples = lookAheadIndexToSamples(mLookAheadIndex);
                break;
            case VX1ExtensionParameterAddress::inputGain:
                mInputGainDb = value;
                mInputGainLinear = std::pow(10.0f, mInputGainDb / 20.0f);
                break;
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        switch (address) {
            case VX1ExtensionParameterAddress::threshold:
                return (AUValue)mThresholdDb;
            case VX1ExtensionParameterAddress::ratio:
                return (AUValue)mRatio;
            case VX1ExtensionParameterAddress::attack:
                return (AUValue)mAttackMs;
            case VX1ExtensionParameterAddress::release:
                return (AUValue)mReleaseMs;
            case VX1ExtensionParameterAddress::makeupGain:
                return (AUValue)mMakeupGainDb;
            case VX1ExtensionParameterAddress::bypass:
                return (AUValue)(mBypassed ? 1.0f : 0.0f);
            case VX1ExtensionParameterAddress::mix:
                return (AUValue)mMixPercent;
            case VX1ExtensionParameterAddress::knee:
                return (AUValue)mKneeDb;
            case VX1ExtensionParameterAddress::detection:
                return (AUValue)mDetectionPercent;
            case VX1ExtensionParameterAddress::sheen:
                return (AUValue)mSheenPercent;
            case VX1ExtensionParameterAddress::autoMakeup:
                return (AUValue)(mAutoMakeupEnabled ? 1.0f : 0.0f);
            case VX1ExtensionParameterAddress::gainReductionMeter:
                return (AUValue)mCurrentGainReductionDb;
            case VX1ExtensionParameterAddress::lookAhead:
                return (AUValue)mLookAheadIndex;
            case VX1ExtensionParameterAddress::inputGain:
                return (AUValue)mInputGainDb;
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

    // MARK: - Look-Ahead Helper

    /// Returns the look-ahead delay in samples for the given step index.
    /// Steps: 0=Off (0ms), 1=2ms, 2=5ms, 3=10ms
    int lookAheadIndexToSamples(int index) const {
        static const float kStepMs[4] = { 0.0f, 2.0f, 5.0f, 10.0f };
        int clamped = std::max(0, std::min(3, index));
        return static_cast<int>(kStepMs[clamped] * 0.001f * mSampleRate);
    }

    /// Returns the current look-ahead time in seconds (used for host latency reporting).
    double lookAheadSeconds() const {
        return lookAheadIndexToSamples(mLookAheadIndex) / mSampleRate;
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
        float drive     = 1.0f + blend * 3.0f;    // 1.0 at 0% → 4.0 at 100%
        float dcOffset  = 0.08f * blend;           // offset grows with Sheen amount
        float driven    = (emphasized + dcOffset) * drive * 1.3f;
        float shaped    = std::tanh(driven);
        float shapedDc  = std::tanh(dcOffset * drive * 1.3f);
        shaped -= shapedDc;                        // remove DC from asymmetry

        // --- Stage 3: Cubic grit layer (3rd harmonic — "edge") ---
        // x^3 generates 3rd harmonic (two octaves up), sits in 4–12 kHz for vocals.
        // Low blend keeps it subtle — adds "cuts through glass" without harshness.
        float cubic    = shaped * shaped * shaped;
        float withGrit = shaped + cubic * 0.06f * blend;

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
        // tanh reduces level at higher drive settings. Previous code applied
        // (1/drive)*1.2 which left the wet path ~10 dB too quiet at Sheen=100%.
        // This formula keeps the wet path near unity at all Sheen settings.
        float compensationGain = 1.0f / (0.5f + 0.5f * blend);
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

            const int lookAheadSamples = mLookAheadSamples;
            const bool usingLookAhead = (lookAheadSamples > 0) && !mRingBuffer.empty();
            const int ringSize = usingLookAhead ? static_cast<int>(mRingBuffer[0].size()) : 0;

            // Process each frame
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {

                // --- Detection: always runs on the current (undelayed) input ---
                // Input gain applied here so detector sees the hotter driven signal,
                // forcing more gain reduction at any threshold/ratio setting.
                // Sidechain signal: mono sum (with input gain) → fixed 80 Hz HPF
                float monoSC = 0.0f;
                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    monoSC += inputBuffers[channel][frameIndex] * mInputGainLinear;
                }
                monoSC /= (float)inputBuffers.size();
                float filteredSC = applyHpf(monoSC);
                float absFiltered = std::abs(filteredSC);

                // Peak and RMS both derived from HPF-filtered mono sidechain
                float peak = absFiltered;
                float rms  = absFiltered;

                // Blend between RMS and Peak based on detection parameter
                // 0% = pure Peak, 100% = pure RMS
                float detectionBlend = mDetectionPercent / 100.0f;
                float detectionLevel = (peak * (1.0f - detectionBlend)) + (rms * detectionBlend);

                // Envelope follower (attack/release)
                float coeff = (detectionLevel > mEnvelopeLevel) ? mAttackCoeff : mReleaseCoeff;
                mEnvelopeLevel = coeff * mEnvelopeLevel + (1.0f - coeff) * detectionLevel;

                // Calculate gain reduction with soft knee
                float gainReduction = 1.0f;
                float gainReductionDb = 0.0f;
                float envelopeDb = 20.0f * std::log10(std::max(mEnvelopeLevel, 1e-6f));
                float overThresholdDb = envelopeDb - mThresholdDb;

                if (mKneeDb > 0.0f && overThresholdDb > -mKneeDb / 2.0f && overThresholdDb < mKneeDb / 2.0f) {
                    // Soft knee region: quadratic interpolation
                    float kneeInput = overThresholdDb + mKneeDb / 2.0f;
                    gainReductionDb = (kneeInput * kneeInput) / (2.0f * mKneeDb) * (1.0f - 1.0f / mRatio);
                    gainReduction = std::pow(10.0f, -gainReductionDb / 20.0f);
                } else if (overThresholdDb > mKneeDb / 2.0f) {
                    // Above knee: full compression
                    gainReductionDb = overThresholdDb * (1.0f - 1.0f / mRatio);
                    gainReduction = std::pow(10.0f, -gainReductionDb / 20.0f);
                }
                // else: below knee, gainReduction stays at 1.0

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

                // Track average gain reduction for auto makeup (smoothed with slow release)
                // Time constant of ~100ms for smooth tracking
                float avgCoeff = std::exp(-1.0f / (100.0f * 0.001f * mSampleRate));
                mAvgGainReductionDb = avgCoeff * mAvgGainReductionDb + (1.0f - avgCoeff) * gainReductionDb;

                // Track peak gain reduction for metering (includes overshoot — meter shows what you hear)
                peakGainReductionDb = std::max(peakGainReductionDb, totalGainReductionDb);

                // Calculate total makeup gain (manual + auto)
                float totalMakeupGainLinear = mMakeupGainLinear;
                if (mAutoMakeupEnabled) {
                    // Auto makeup compensates 80% of average gain reduction
                    float autoMakeupDb = mAvgGainReductionDb * 0.8f;
                    float autoMakeupLinear = std::pow(10.0f, autoMakeupDb / 20.0f);
                    totalMakeupGainLinear *= autoMakeupLinear;
                }

                // Apply compression, saturation, makeup gain, then mix with dry signal
                float mixWet = mMixPercent / 100.0f;
                float mixDry = 1.0f - mixWet;

                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    // Apply input gain to audio path (matches what detector saw)
                    float currentInput = inputBuffers[channel][frameIndex] * mInputGainLinear;

                    // --- Look-ahead: audio path uses the delayed sample from the ring buffer ---
                    float audioInput;
                    if (usingLookAhead) {
                        // Read the oldest sample (delayed by lookAheadSamples)
                        int readPos = (mRingWritePos - lookAheadSamples + ringSize) % ringSize;
                        audioInput = mRingBuffer[channel][readPos];
                        // Write the gained input into the ring buffer
                        mRingBuffer[channel][mRingWritePos] = currentInput;
                    } else {
                        audioInput = currentInput;
                    }

                    // Apply compression with VCA overshoot (total GR = compressor GR + overshoot)
                    float compressed = audioInput * gainReductionTotal;

                    // Apply sheen saturation (presence-biased harmonic coloration)
                    float saturated = applySaturation(compressed, mSheenPercent, (int)channel);

                    // Apply makeup gain (manual + auto)
                    saturated *= totalMakeupGainLinear;

                    // Parallel mix: blend dry and processed signals
                    // Dry signal is also the delayed audio so timing stays aligned
                    float output = (audioInput * mixDry) + (saturated * mixWet);
                    outputBuffers[channel][frameIndex] = output;
                }

                // Advance ring buffer write position once per frame (after all channels)
                if (usingLookAhead) {
                    mRingWritePos = (mRingWritePos + 1) % ringSize;
                }
            }

            // Update meter with peak gain reduction from this buffer
            // Apply smoothing for visual stability (fast attack, adaptive release)
            float meterAttackCoeff = 0.3f;  // Fast attack for meter

            if (peakGainReductionDb > mCurrentGainReductionDb) {
                // Attack - respond quickly to increases
                mCurrentGainReductionDb = meterAttackCoeff * mCurrentGainReductionDb + (1.0f - meterAttackCoeff) * peakGainReductionDb;
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
    float mThresholdDb = -20.0f;
    float mRatio = 4.0f;
    float mAttackMs = 10.0f;
    float mReleaseMs = 100.0f;
    float mMakeupGainDb = 0.0f;
    float mMixPercent = 100.0f;
    float mKneeDb = 3.0f;
    float mDetectionPercent = 100.0f;  // 0% = Peak, 100% = RMS
    float mSheenPercent = 25.0f;       // 0% = Clean, 100% = Heavy sheen/presence saturation
    bool mAutoMakeupEnabled = false;   // Auto makeup gain toggle
    float mInputGainDb = 0.0f;         // Pre-compression input gain: 0 to +24 dB

    // Computed/cached values (linear)
    float mThresholdLinear = 0.1f;  // 10^(thresholdDb/20)
    float mMakeupGainLinear = 1.0f; // 10^(makeupGainDb/20)
    float mInputGainLinear  = 1.0f; // 10^(inputGainDb/20)
    float mAttackCoeff = 0.0f;      // exp(-1/(attackMs * 0.001 * sampleRate))
    float mReleaseCoeff = 0.0f;     // exp(-1/(releaseMs * 0.001 * sampleRate))

    // State
    float mEnvelopeLevel = 0.0f;           // Envelope follower state
    float mAvgGainReductionDb = 0.0f;      // Smoothed average gain reduction for auto makeup
    float mCurrentGainReductionDb = 0.0f;  // Current gain reduction for metering

    // Look-ahead
    int mLookAheadIndex = 0;               // 0=Off, 1=2ms, 2=5ms, 3=10ms
    int mLookAheadSamples = 0;             // Derived sample count from index
    int mChannelCount = 2;                 // Set during initialize()
    std::vector<std::vector<float>> mRingBuffer; // Per-channel delay line [channel][sample]
    int mRingWritePos = 0;                 // Current write position in ring buffer

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
};
