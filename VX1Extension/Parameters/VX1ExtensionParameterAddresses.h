//
//  VX1ExtensionParameterAddresses.h
//  VX1Extension
//
//  Created by Taylor Page on 1/22/26.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, VX1ExtensionParameterAddress) {
    compress = 0,             // Combined threshold+ratio control: 0% = no compression, 100% = max compression
    attack = 2,
    release = 3,
    makeupGain = 4,
    bypass = 5,
    mix = 6,
    grip = 8,
    bite = 9,
    stack = 10,               // Double-compression blend: 0% = single pass, 100% = double pass
    gainReductionMeter = 11,  // Read-only meter value
    gateThreshold = 14        // Noise gate threshold: -80 to -20 dB (-80 dB default = off)
};
