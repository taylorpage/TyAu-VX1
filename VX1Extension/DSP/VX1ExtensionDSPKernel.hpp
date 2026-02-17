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
        } else {
            // Process each frame
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                // Calculate RMS level across all channels
                float sumSquares = 0.0f;
                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    float sample = inputBuffers[channel][frameIndex];
                    sumSquares += sample * sample;
                }
                float rms = std::sqrt(sumSquares / inputBuffers.size());

                // Envelope follower (attack/release)
                float coeff = (rms > mEnvelopeLevel) ? mAttackCoeff : mReleaseCoeff;
                mEnvelopeLevel = coeff * mEnvelopeLevel + (1.0f - coeff) * rms;

                // Calculate gain reduction
                float gainReduction = 1.0f;
                if (mEnvelopeLevel > mThresholdLinear) {
                    // Calculate how much we're over the threshold in dB
                    float envelopeDb = 20.0f * std::log10(std::max(mEnvelopeLevel, 1e-6f));
                    float overThresholdDb = envelopeDb - mThresholdDb;

                    // Apply ratio (compress the overage)
                    float gainReductionDb = overThresholdDb * (1.0f - 1.0f / mRatio);

                    // Convert back to linear
                    gainReduction = std::pow(10.0f, -gainReductionDb / 20.0f);
                }

                // Apply compression and makeup gain to all channels
                for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                    float input = inputBuffers[channel][frameIndex];
                    float output = input * gainReduction * mMakeupGainLinear;
                    outputBuffers[channel][frameIndex] = output;
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

    // Computed/cached values (linear)
    float mThresholdLinear = 0.1f;  // 10^(thresholdDb/20)
    float mMakeupGainLinear = 1.0f; // 10^(makeupGainDb/20)
    float mAttackCoeff = 0.0f;      // exp(-1/(attackMs * 0.001 * sampleRate))
    float mReleaseCoeff = 0.0f;     // exp(-1/(releaseMs * 0.001 * sampleRate))

    // State
    float mEnvelopeLevel = 0.0f;    // Envelope follower state
};
