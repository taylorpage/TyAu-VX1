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
            valueRange: -50.0...0.0,
            defaultValue: -20.0
        )
        ParameterSpec(
            address: .ratio,
            identifier: "ratio",
            name: "Ratio",
            units: .ratio,
            valueRange: 1.0...30.0,
            defaultValue: 4.0
        )
        ParameterSpec(
            address: .attack,
            identifier: "attack",
            name: "Attack",
            units: .milliseconds,
            valueRange: 0.0...200.0,
            defaultValue: 10.0
        )
        ParameterSpec(
            address: .release,
            identifier: "release",
            name: "Release",
            units: .milliseconds,
            valueRange: 5.0...5000.0,
            defaultValue: 100.0
        )
        ParameterSpec(
            address: .makeupGain,
            identifier: "makeupGain",
            name: "Makeup Gain",
            units: .decibels,
            valueRange: -20.0...50.0,
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
        ParameterSpec(
            address: .mix,
            identifier: "mix",
            name: "Mix",
            units: .percent,
            valueRange: 0.0...100.0,
            defaultValue: 100.0
        )
        ParameterSpec(
            address: .grip,
            identifier: "grip",
            name: "Grip",
            units: .percent,
            valueRange: 0.0...100.0,
            defaultValue: 0.0
        )
        ParameterSpec(
            address: .bite,
            identifier: "bite",
            name: "Bite",
            units: .percent,
            valueRange: 0.0...100.0,
            defaultValue: 25.0
        )
        ParameterSpec(
            address: .stack,
            identifier: "stack",
            name: "Stack",
            units: .percent,
            valueRange: 0.0...100.0,
            defaultValue: 0.0
        )
        ParameterSpec(
            address: .gainReductionMeter,
            identifier: "gainReductionMeter",
            name: "Gain Reduction",
            units: .decibels,
            valueRange: 0.0...60.0,
            defaultValue: 0.0,
            flags: [.flag_IsReadable, .flag_IsWritable]  // Writable for internal updates, but controlled by DSP
        )
        ParameterSpec(
            address: .gateThreshold,
            identifier: "gateThreshold",
            name: "Gate",
            units: .decibels,
            valueRange: -80.0...(-20.0),
            defaultValue: -80.0
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
