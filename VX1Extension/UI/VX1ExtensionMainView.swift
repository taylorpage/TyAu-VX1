//
//  VX1ExtensionMainView.swift
//  VX1Extension
//
//  Compressor plugin UI for TyAu-VX1
//

import SwiftUI

struct VX1ExtensionMainView: View {
    var parameterTree: ObservableAUParameterGroup
    var meterValueProvider: () -> Float

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

            VStack(spacing: 0) {
                // Title
                Text("VX1 COMPRESSOR")
                    .font(.system(size: 18, weight: .bold))
                    .foregroundColor(.white)
                    .padding(.top, 4)
                    .padding(.bottom, 4)

                // VU Meter (gain reduction) — bleeds to chassis edges
                GeometryReader { geo in
                    VUMeter(valueProvider: meterValueProvider, width: geo.size.width - 60, height: 160)
                        .offset(x: 30)
                }
                .frame(height: 160)

                // Main controls grid
                VStack(spacing: 0) {

                    // Row 1: Gate, Speed, Makeup — small utility knobs across the top
                    HStack(spacing: 12) {
                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.gateThreshold, size: 52)
                            Text("GATE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(Color(red: 1.0, green: 0.6, blue: 0.2))
                                .padding(.top, -20)
                        }

                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.speed, size: 52)
                            Text("SPEED")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -20)
                        }

                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.makeupGain, size: 52)
                            Text("MAKEUP")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -20)
                        }
                    }
                    .padding(.top, 6)

                    // Row 2: Stack — large, centered, solo
                    VStack(spacing: 0) {
                        ParameterKnob(param: parameterTree.global.stack, size: 90)
                        Text("STACK")
                            .font(.system(size: 8, weight: .semibold))
                            .foregroundColor(Color(red: 0.4, green: 0.8, blue: 1.0))
                            .padding(.top, -34)
                    }
                    .padding(.top, -30)

                    // Row 3: Grip (left), Bite (right) — medium character knobs
                    HStack(spacing: 40) {
                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.grip, size: 68)
                            Text("GRIP")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -26)
                        }

                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.bite, size: 68)
                            Text("BITE")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -26)
                        }
                    }
                    .padding(.top, -45)

                    // Row 4: Compress (hero) + Mix (bottom-right corner)
                    ZStack(alignment: .bottomTrailing) {
                        HStack {
                            Spacer()
                            VStack(spacing: 0) {
                                ParameterKnob(param: parameterTree.global.compress, size: 150)
                                Text("COMPRESS")
                                    .font(.system(size: 8, weight: .semibold))
                                    .foregroundColor(.white.opacity(0.8))
                                    .padding(.top, -54)
                            }
                            Spacer()
                        }

                        VStack(spacing: 0) {
                            ParameterKnob(param: parameterTree.global.mix, size: 44)
                            Text("MIX")
                                .font(.system(size: 8, weight: .semibold))
                                .foregroundColor(.white.opacity(0.8))
                                .padding(.top, -17)
                        }
                        .offset(x: 4, y: -2)
                    }
                    .padding(.top, -65)

                }

                // Logo
                Text("TaylorAudio")
                    .font(.system(size: 9))
                    .foregroundColor(.gray)
                    .padding(.top, 4)
                    .padding(.bottom, 4)
            }
            .padding(.horizontal, 6)
        }
        .frame(width: 260, height: 520)
    }

    // MARK: - Computed Properties

    var bypassParam: ObservableAUParameter {
        parameterTree.global.bypass
    }
}
