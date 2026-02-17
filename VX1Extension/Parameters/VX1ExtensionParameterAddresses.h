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
    drive = 9,
    autoMakeup = 10,
    gainReductionMeter = 11  // Read-only meter value
};
