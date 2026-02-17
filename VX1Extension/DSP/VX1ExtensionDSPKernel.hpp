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

#include "VX1ExtensionParameterAddresses.h"

/*
 VX1ExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.
 */
class VX1ExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;

        // Initialize computed coefficients
        mThresholdLinear = std::pow(10.0f, mThresholdDb / 20.0f);
        mMakeupGainLinear = std::pow(10.0f, mMakeupGainDb / 20.0f);
        mAttackCoeff = std::exp(-1.0f / (mAttackMs * 0.001f * mSampleRate));
        mReleaseCoeff = std::exp(-1.0f / (mReleaseMs * 0.001f * mSampleRate));

        // Reset state
        mEnvelopeLevel = 0.0f;
    }

    void deInitialize() {
        // Reset all state when deallocating
        mEnvelopeLevel = 0.0f;
        mAvgGainReductionDb = 0.0f;
        mCurrentGainReductionDb = 0.0f;
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
            case VX1ExtensionParameterAddress::drive:
                mDrivePercent = value;
                break;
            case VX1ExtensionParameterAddress::autoMakeup:
                mAutoMakeupEnabled = (value >= 0.5f);
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
            case VX1ExtensionParameterAddress::drive:
                return (AUValue)mDrivePercent;
            case VX1ExtensionParameterAddress::autoMakeup:
                return (AUValue)(mAutoMakeupEnabled ? 1.0f : 0.0f);
            case VX1ExtensionParameterAddress::gainReductionMeter:
                return (AUValue)mCurrentGainReductionDb;
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

    // MARK: - Saturation
    /**
     Soft tape-style saturation for harmonic enhancement.
     Uses a combination of tanh and soft clipping for warm, musical distortion.
     */
    float applySaturation(float input, float amount) {
        if (amount <= 0.0f) return input;

        // Scale amount (0-100%) to drive factor (1.0-4.0)
        float drive = 1.0f + (amount / 100.0f) * 3.0f;

        // Apply input gain
        float driven = input * drive;

        // Soft tape saturation using tanh with asymmetry
        float saturated = std::tanh(driven * 1.5f);

        // Add subtle even harmonics for "sheen"
        saturated += std::tanh(driven * 2.0f) * 0.15f;

        // Compensate for level boost
        saturated *= (1.0f / drive) * 1.2f;

        // Blend with dry signal based on amount
        float blend = amount / 100.0f;
        return input * (1.0f - blend) + saturated * blend;
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
            // Reset all state when bypassed
            mCurrentGainReductionDb = 0.0f;
            mAvgGainReductionDb = 0.0f;
            mEnvelopeLevel = 0.0f;
        } else {
            // First, check if the entire buffer is essentially silent
            // This allows us to quickly reset state when audio stops
            bool bufferIsSilent = true;
            for (UInt32 channel = 0; channel < inputBuffers.size() && bufferIsSilent; ++channel) {
                for (UInt32 i = 0; i < frameCount; ++i) {
                    if (std::abs(inputBuffers[channel][i]) > 1e-6f) {
                        bufferIsSilent = false;
                        break;
                    }
                }
            }

            // If buffer is silent, reset state immediately and pass through
            if (bufferIsSilent) {
                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    std::copy_n(inputBuffers[channel], frameCount, outputBuffers[channel]);
                }
                mEnvelopeLevel = 0.0f;
                mAvgGainReductionDb = 0.0f;
                mCurrentGainReductionDb = 0.0f;
                return;
            }

            // Track peak gain reduction in this buffer
            float peakGainReductionDb = 0.0f;

            // Process each frame
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                // Calculate both peak and RMS across all channels
                float peak = 0.0f;
                float sumSquares = 0.0f;
                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    float sample = std::abs(inputBuffers[channel][frameIndex]);
                    peak = std::max(peak, sample);
                    sumSquares += sample * sample;
                }
                float rms = std::sqrt(sumSquares / inputBuffers.size());

                // Blend between RMS and Peak based on detection parameter
                // 0% = pure Peak, 100% = pure RMS
                float detectionBlend = mDetectionPercent / 100.0f;
                float detectionLevel = (peak * (1.0f - detectionBlend)) + (rms * detectionBlend);

                // Envelope follower (attack/release)
                // Use faster release coefficient when input is essentially silent
                // This allows the meter to reset quickly when audio stops
                float coeff;
                if (detectionLevel > mEnvelopeLevel) {
                    coeff = mAttackCoeff;  // Attack
                } else if (detectionLevel < 1e-5f) {
                    // Input is essentially silent - reset envelope very quickly
                    mEnvelopeLevel = 0.0f;
                    coeff = 0.0f;  // Will be ignored since we set envelope to 0
                } else {
                    coeff = mReleaseCoeff;  // Normal release
                }

                if (detectionLevel >= 1e-5f) {
                    mEnvelopeLevel = coeff * mEnvelopeLevel + (1.0f - coeff) * detectionLevel;
                }

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

                // Track average gain reduction for auto makeup (smoothed with slow release)
                // Time constant of ~100ms for smooth tracking
                float avgCoeff = std::exp(-1.0f / (100.0f * 0.001f * mSampleRate));
                mAvgGainReductionDb = avgCoeff * mAvgGainReductionDb + (1.0f - avgCoeff) * gainReductionDb;

                // Track peak gain reduction for metering
                peakGainReductionDb = std::max(peakGainReductionDb, gainReductionDb);

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
                    float input = inputBuffers[channel][frameIndex];

                    // Apply compression
                    float compressed = input * gainReduction;

                    // Apply saturation to compressed signal
                    float saturated = applySaturation(compressed, mDrivePercent);

                    // Apply makeup gain (manual + auto)
                    saturated *= totalMakeupGainLinear;

                    // Parallel mix: blend dry and processed signals
                    float output = (input * mixDry) + (saturated * mixWet);
                    outputBuffers[channel][frameIndex] = output;
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
    float mDrivePercent = 25.0f;       // 0% = Clean, 100% = Heavy saturation
    bool mAutoMakeupEnabled = false;   // Auto makeup gain toggle

    // Computed/cached values (linear)
    float mThresholdLinear = 0.1f;  // 10^(thresholdDb/20)
    float mMakeupGainLinear = 1.0f; // 10^(makeupGainDb/20)
    float mAttackCoeff = 0.0f;      // exp(-1/(attackMs * 0.001 * sampleRate))
    float mReleaseCoeff = 0.0f;     // exp(-1/(releaseMs * 0.001 * sampleRate))

    // State
    float mEnvelopeLevel = 0.0f;    // Envelope follower state
    float mAvgGainReductionDb = 0.0f;  // Smoothed average gain reduction for auto makeup
    float mCurrentGainReductionDb = 0.0f;  // Current gain reduction for metering
};
