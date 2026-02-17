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

            VStack(spacing: 15) {
                // Title
                Text("VX1 COMPRESSOR")
                    .font(.system(size: 20, weight: .bold))
                    .foregroundColor(.white)
                    .padding(.top, 15)

                // LED indicator
                Circle()
                    .fill(bypassParam.boolValue ? Color.gray : Color(red: 0.2, green: 0.8, blue: 1.0))
                    .frame(width: 10, height: 10)
                    .shadow(color: bypassParam.boolValue ? .clear : Color(red: 0.2, green: 0.8, blue: 1.0).opacity(0.8), radius: 6)

                // Main controls grid
                VStack(spacing: 20) {
                    // Top row: Threshold and Ratio
                    HStack(spacing: 30) {
                        VStack(spacing: 8) {
                            ParameterKnob(param: parameterTree.global.threshold, size: 80)
                            Text("THRESHOLD")
                                .font(.system(size: 11, weight: .semibold))
                                .foregroundColor(.white.opacity(0.9))
                        }

                        VStack(spacing: 8) {
                            ParameterKnob(param: parameterTree.global.ratio, size: 80)
                            Text("RATIO")
                                .font(.system(size: 11, weight: .semibold))
                                .foregroundColor(.white.opacity(0.9))
                        }
                    }

                    // Middle row: Attack and Release
                    HStack(spacing: 30) {
                        VStack(spacing: 8) {
                            ParameterKnob(param: parameterTree.global.attack, size: 80)
                            Text("ATTACK")
                                .font(.system(size: 11, weight: .semibold))
                                .foregroundColor(.white.opacity(0.9))
                        }

                        VStack(spacing: 8) {
                            ParameterKnob(param: parameterTree.global.release, size: 80)
                            Text("RELEASE")
                                .font(.system(size: 11, weight: .semibold))
                                .foregroundColor(.white.opacity(0.9))
                        }
                    }

                    // Bottom row: Makeup Gain
                    VStack(spacing: 8) {
                        ParameterKnob(param: parameterTree.global.makeupGain, size: 90)
                        Text("MAKEUP GAIN")
                            .font(.system(size: 12, weight: .semibold))
                            .foregroundColor(.white.opacity(0.9))
                    }
                }
                .padding(.vertical, 10)

                Spacer()

                // Bypass button
                BypassButton(param: parameterTree.global.bypass)

                // Logo
                Text("TaylorAudio")
                    .font(.caption)
                    .foregroundColor(.gray)
                    .padding(.bottom, 15)
            }
            .padding()
        }
        .frame(width: 400, height: 550)
    }

    // MARK: - Computed Properties

    var bypassParam: ObservableAUParameter {
        parameterTree.global.bypass
    }
}
