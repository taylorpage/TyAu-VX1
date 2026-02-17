//
//  VX1ExtensionMainView.swift
//  VX1Extension
//
//  Compressor plugin UI for TyAu-VX1
//

import SwiftUI

struct VX1ExtensionMainView: View {
    var parameterTree: ObservableAUParameterGroup

    var body: some View {
        ZStack {
            // Dark gradient background for professional compressor look
            LinearGradient(
                gradient: Gradient(colors: [
                    Color(red: 0.15, green: 0.15, blue: 0.18),
                    Color(red: 0.10, green: 0.10, blue: 0.12)
                ]),
                startPoint: .top,
                endPoint: .bottom
            )
            .cornerRadius(8)

            VStack(spacing: 8) {
                // Title
                Text("VX1 COMPRESSOR")
                    .font(.system(size: 18, weight: .bold))
                    .foregroundColor(.white)
                    .padding(.top, 10)

                // LED indicator
                Circle()
                    .fill(bypassParam.boolValue ? Color.gray : Color(red: 0.2, green: 0.8, blue: 1.0))
                    .frame(width: 8, height: 8)
                    .shadow(color: bypassParam.boolValue ? .clear : Color(red: 0.2, green: 0.8, blue: 1.0).opacity(0.8), radius: 4)

                // Gain Reduction Meter
                VStack(spacing: 2) {
                    Text("GAIN REDUCTION")
                        .font(.system(size: 8, weight: .semibold))
                        .foregroundColor(.white.opacity(0.8))
                    GainReductionMeter(param: gainReductionParam)
                }
                .padding(.vertical, 8)

                // Main controls grid
                VStack(spacing: 8) {
                    // Top row: Gate, Input, Threshold, Ratio
                    HStack(spacing: 10) {
                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.gateThreshold, size: 55)
                            Text("GATE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(Color(red: 1.0, green: 0.6, blue: 0.2))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.inputGain, size: 55)
                            Text("INPUT")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(Color(red: 0.2, green: 0.8, blue: 1.0))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.threshold, size: 55)
                            Text("THRESHOLD")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.ratio, size: 55)
                            Text("RATIO")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }
                    }

                    // Second row: Attack and Release
                    HStack(spacing: 15) {
                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.attack, size: 65)
                            Text("ATTACK")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.release, size: 65)
                            Text("RELEASE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }
                    }

                    // Third row: Mix, Knee, and Detection
                    HStack(spacing: 15) {
                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.mix, size: 65)
                            Text("MIX")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.knee, size: 65)
                            Text("KNEE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.detection, size: 65)
                            Text("DETECT")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }
                    }

                    // Fourth row: Makeup Gain and Sheen
                    HStack(spacing: 15) {
                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.makeupGain, size: 65)
                            Text("MAKEUP")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))

                            // Auto makeup toggle
                            Toggle(isOn: Binding(
                                get: { autoMakeupParam.boolValue },
                                set: { newValue in
                                    autoMakeupParam.onEditingChanged(true)
                                    autoMakeupParam.boolValue = newValue
                                    autoMakeupParam.onEditingChanged(false)
                                }
                            )) {
                                Text("AUTO")
                                    .font(.system(size: 7, weight: .medium))
                                    .foregroundColor(.white.opacity(0.7))
                            }
                            .toggleStyle(.switch)
                            .scaleEffect(0.6)
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.sheen, size: 65)
                            Text("SHEEN")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }
                    }

                    // Fifth row: Look-Ahead
                    VStack(spacing: 4) {
                        Text("LOOK-AHEAD")
                            .font(.system(size: 8, weight: .semibold))
                            .foregroundColor(.white.opacity(0.8))
                        Picker("", selection: Binding(
                            get: { Int(lookAheadParam.value) },
                            set: { newValue in
                                lookAheadParam.onEditingChanged(true)
                                lookAheadParam.value = Float(newValue)
                                lookAheadParam.onEditingChanged(false)
                            }
                        )) {
                            Text("Off").tag(0)
                            Text("2ms").tag(1)
                            Text("5ms").tag(2)
                            Text("10ms").tag(3)
                        }
                        .pickerStyle(.segmented)
                        .frame(width: 220)
                        .colorMultiply(Color(red: 0.2, green: 0.8, blue: 1.0))
                    }
                    .padding(.top, 4)
                }
                .padding(.vertical, 5)

                Spacer()

                // Bypass button
                BypassButton(param: parameterTree.global.bypass)

                // Logo
                Text("TaylorAudio")
                    .font(.system(size: 9))
                    .foregroundColor(.gray)
                    .padding(.bottom, 8)
            }
            .padding(12)
        }
        .frame(width: 350, height: 580)
    }

    // MARK: - Computed Properties

    var bypassParam: ObservableAUParameter {
        parameterTree.global.bypass
    }

    var autoMakeupParam: ObservableAUParameter {
        parameterTree.global.autoMakeup
    }

    var gainReductionParam: ObservableAUParameter {
        parameterTree.global.gainReductionMeter
    }

    var lookAheadParam: ObservableAUParameter {
        parameterTree.global.lookAhead
    }
}
