//
//  Parameters.swift
//  VX1Extension
//
//  Created by Taylor Page on 1/22/26.
//

import Foundation
import AudioToolbox

let VX1ExtensionParameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
            address: .threshold,
            identifier: "threshold",
            name: "Threshold",
            units: .decibels,
            valueRange: -60.0...0.0,
            defaultValue: -20.0
        )
        ParameterSpec(
            address: .ratio,
            identifier: "ratio",
            name: "Ratio",
            units: .ratio,
            valueRange: 1.0...20.0,
            defaultValue: 4.0
        )
        ParameterSpec(
            address: .attack,
            identifier: "attack",
            name: "Attack",
            units: .milliseconds,
            valueRange: 0.1...100.0,
            defaultValue: 10.0
        )
        ParameterSpec(
            address: .release,
            identifier: "release",
            name: "Release",
            units: .milliseconds,
            valueRange: 10.0...1000.0,
            defaultValue: 100.0
        )
        ParameterSpec(
            address: .makeupGain,
            identifier: "makeupGain",
            name: "Makeup Gain",
            units: .decibels,
            valueRange: 0.0...24.0,
            defaultValue: 0.0
        )
        ParameterSpec(
            address: .bypass,
            identifier: "bypass",
            name: "Bypass",
            units: .boolean,
            valueRange: 0.0...1.0,
            defaultValue: 0.0
        )
    }
}

extension ParameterSpec {
    init(
        address: VX1ExtensionParameterAddress,
        identifier: String,
        name: String,
        units: AudioUnitParameterUnit,
        valueRange: ClosedRange<AUValue>,
        defaultValue: AUValue,
        unitName: String? = nil,
        flags: AudioUnitParameterOptions = [AudioUnitParameterOptions.flag_IsWritable, AudioUnitParameterOptions.flag_IsReadable],
        valueStrings: [String]? = nil,
        dependentParameters: [NSNumber]? = nil
    ) {
        self.init(address: address.rawValue,
                  identifier: identifier,
                  name: name,
                  units: units,
                  valueRange: valueRange,
                  defaultValue: defaultValue,
                  unitName: unitName,
                  flags: flags,
                  valueStrings: valueStrings,
                  dependentParameters: dependentParameters)
    }
}
