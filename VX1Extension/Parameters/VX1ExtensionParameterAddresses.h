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
    bypass = 5
};
