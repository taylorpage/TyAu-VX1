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

                // VU Meter (gain reduction)
                VUMeter(param: gainReductionParam, width: 300, height: 160)
                    .padding(.vertical, 4)

                // Main controls grid
                VStack(spacing: 8) {

                    // Row 1: Stack — solo, largest knob
                    HStack {
                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.stack, size: 80)
                            Text("STACK")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(Color(red: 0.4, green: 0.8, blue: 1.0))
                                .padding(.top, -28)
                        }
                    }

                    // Row 2: Grip and Bite — character controls
                    HStack(spacing: 15) {
                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.grip, size: 70)
                            Text("GRIP")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -26)
                        }

                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.bite, size: 70)
                            Text("BITE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -26)
                        }
                    }

                    // Rows 3a/3b: Core compression controls — grouped tightly
                    VStack(spacing: 4) {
                        HStack(spacing: 15) {
                            VStack(spacing: 0) {
                                ParameterKnob(param: parameterTree.global.compress, size: 65)
                                Text("COMPRESS")
                                    .font(.system(size: 8, weight: .semibold))
                                    .foregroundColor(.white.opacity(0.8))
                                    .padding(.top, -22)
                            }
                        }

                        HStack(spacing: 15) {
                            VStack(spacing: 0) {
                                ParameterKnob(param: parameterTree.global.attack, size: 65)
                                Text("ATTACK")
                                    .font(.system(size: 8, weight: .semibold))
                                    .foregroundColor(.white.opacity(0.8))
                                    .padding(.top, -22)
                            }

                            VStack(spacing: 0) {
                                ParameterKnob(param: parameterTree.global.release, size: 65)
                                Text("RELEASE")
                                    .font(.system(size: 8, weight: .semibold))
                                    .foregroundColor(.white.opacity(0.8))
                                    .padding(.top, -22)
                            }
                        }
                    }

                    // Row 4a: Gate — solo utility
                    HStack {
                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.gateThreshold, size: 55)
                            Text("GATE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(Color(red: 1.0, green: 0.6, blue: 0.2))
                                .padding(.top, -20)
                        }
                    }

                    // Row 4b: Makeup and Mix — utility pair
                    HStack(spacing: 15) {
                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.makeupGain, size: 55)
                            Text("MAKEUP")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -20)
                        }

                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.mix, size: 55)
                            Text("MIX")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -20)
                        }
                    }

                }
                .padding(.vertical, 5)

                Spacer()

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
