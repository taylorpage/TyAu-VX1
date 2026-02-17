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

                // VU Meter (gain reduction)
                VUMeter(param: gainReductionParam, width: 300, height: 160)
                    .padding(.vertical, 4)

                // Main controls grid
                VStack(spacing: 8) {
                    // Top row: Gate, Threshold, Ratio
                    HStack(spacing: 10) {
                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.gateThreshold, size: 55)
                            Text("GATE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(Color(red: 1.0, green: 0.6, blue: 0.2))
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

                    // Third row: Mix and Grip
                    HStack(spacing: 15) {
                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.mix, size: 65)
                            Text("MIX")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.grip, size: 65)
                            Text("GRIP")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }
                    }

                    // Fourth row: Makeup Gain and Bite
                    HStack(spacing: 15) {
                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.makeupGain, size: 65)
                            Text("MAKEUP")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }

                        VStack(spacing: 2) {
                            ParameterKnob(param: parameterTree.global.bite, size: 65)
                            Text("BITE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                        }
                    }

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
        .frame(width: 350, height: 680)
    }

    // MARK: - Computed Properties

    var bypassParam: ObservableAUParameter {
        parameterTree.global.bypass
    }

    var gainReductionParam: ObservableAUParameter {
        parameterTree.global.gainReductionMeter
    }
}
