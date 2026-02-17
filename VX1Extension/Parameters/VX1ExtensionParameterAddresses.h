//
//  VX1ExtensionParameterAddresses.h
//  VX1Extension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, VX1ExtensionParameterAddress) {
    threshold = 0,
    ratio = 1,
    attack = 2,
    release = 3,
    makeupGain = 4,
    bypass = 5,
    mix = 6,
    knee = 7,
    detection = 8,
    sheen = 9,
    autoMakeup = 10,
    gainReductionMeter = 11,  // Read-only meter value
    lookAhead = 12,           // Look-ahead time: 0=Off, 1=2ms, 2=5ms, 3=10ms
    inputGain = 13,           // Pre-compression input gain: 0 to +24 dB
    gateThreshold = 14,       // Noise gate threshold: -80 to -20 dB (-60 dB default = off)
    outputLevelMeter = 15     // Read-only output level meter: -60 to 0 dB
};
