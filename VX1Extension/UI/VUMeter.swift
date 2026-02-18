//
//  VUMeter.swift
//  VX1Extension
//
//  Vintage analog-style VU meter. Driven by a CADisplayLink that calls a value provider
//  closure each display frame, bypassing the AU parameter polling pipeline entirely.
//

import SwiftUI

struct VUMeter: View {
    /// Called each display frame to get the current gain reduction in dB (0–40).
    let valueProvider: () -> Float

    let width: CGFloat
    let height: CGFloat

    @State private var normalizedValue: Double = 0.0
    @State private var displayLinkHolder: DisplayLinkHolder?

    // Arc configuration - needle sweeps from left to right over 120°
    private let startAngle: Double = 210
    private let endAngle: Double = 330

    init(valueProvider: @escaping () -> Float,
         width: CGFloat = 300,
         height: CGFloat = 180) {
        self.valueProvider = valueProvider
        self.width = width
        self.height = height
    }

    private func needleAngle(for value: Double) -> Angle {
        let totalDegrees = endAngle - startAngle
        let degrees = startAngle + (value * totalDegrees)
        return .degrees(degrees + 90)
    }

    // GR scale labels — 0 dB at left (0.0), increasing GR to the right, max 40 dB at right (1.0)
    private let scaleLabels = [
        (value: 0.0,   label: "0"),
        (value: 0.125, label: "5"),
        (value: 0.25,  label: "10"),
        (value: 0.375, label: "15"),
        (value: 0.5,   label: "20"),
        (value: 0.75,  label: "30"),
        (value: 1.0,   label: "40"),
    ]

    var body: some View {
        let currentValue = normalizedValue
        return ZStack {
            // Outer dark frame/bezel
            RoundedRectangle(cornerRadius: 8)
                .fill(
                    LinearGradient(
                        gradient: Gradient(stops: [
                            .init(color: Color(red: 0.15, green: 0.15, blue: 0.15), location: 0.0),
                            .init(color: Color(red: 0.08, green: 0.08, blue: 0.08), location: 0.5),
                            .init(color: Color(red: 0.12, green: 0.12, blue: 0.12), location: 1.0)
                        ]),
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: width, height: height)
                .shadow(color: .black.opacity(0.5), radius: 8, x: 0, y: 4)

            // Inner bezel edge highlight
            RoundedRectangle(cornerRadius: 8)
                .stroke(
                    LinearGradient(
                        gradient: Gradient(colors: [
                            Color.white.opacity(0.15),
                            Color.clear,
                            Color.black.opacity(0.3)
                        ]),
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    ),
                    lineWidth: 1.5
                )
                .frame(width: width - 2, height: height - 2)

            // Main meter face background (warm cream/beige)
            RoundedRectangle(cornerRadius: 4)
                .fill(
                    LinearGradient(
                        gradient: Gradient(stops: [
                            .init(color: Color(red: 0.95, green: 0.92, blue: 0.85), location: 0.0),
                            .init(color: Color(red: 0.92, green: 0.88, blue: 0.78), location: 0.3),
                            .init(color: Color(red: 0.88, green: 0.84, blue: 0.72), location: 1.0)
                        ]),
                        startPoint: .top,
                        endPoint: .bottom
                    )
                )
                .frame(width: width - 4, height: height - 4)

            // Inner shadow on meter face for depth
            RoundedRectangle(cornerRadius: 4)
                .stroke(
                    LinearGradient(
                        gradient: Gradient(colors: [
                            Color.black.opacity(0.2),
                            Color.clear
                        ]),
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    ),
                    lineWidth: 2
                )
                .frame(width: width - 4, height: height - 4)

            // Meter content
            ZStack {
                redDangerZone
                tickMarks
                dbScaleLabels

                // VU label (bottom right)
                GeometryReader { geometry in
                    Text("VU")
                        .font(.system(size: height * 0.06, weight: .bold, design: .serif))
                        .foregroundColor(Color(red: 0.2, green: 0.2, blue: 0.2))
                        .position(
                            x: geometry.size.width - (width * 0.08),
                            y: geometry.size.height - (height * 0.08)
                        )
                }

                // Branding label
                Text("TyAu")
                    .font(.system(size: height * 0.07, weight: .medium, design: .serif))
                    .foregroundColor(Color(red: 0.3, green: 0.3, blue: 0.3))
                    .offset(y: height * 0.25)

                needleView(value: currentValue)
            }
            .frame(width: width - 4, height: height - 4)
            .clipShape(RoundedRectangle(cornerRadius: 4))

            // Glass/glare overlay
            RoundedRectangle(cornerRadius: 4)
                .fill(
                    LinearGradient(
                        gradient: Gradient(stops: [
                            .init(color: Color.white.opacity(0.15), location: 0.0),
                            .init(color: Color.white.opacity(0.05), location: 0.3),
                            .init(color: Color.clear, location: 0.6)
                        ]),
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: width - 4, height: height - 4)
                .blendMode(.overlay)
        }
        .frame(width: width, height: height)
        .onAppear {
            let holder = DisplayLinkHolder(valueProvider: valueProvider) { newValue in
                normalizedValue = newValue
            }
            displayLinkHolder = holder
            holder.start()
        }
        .onDisappear {
            displayLinkHolder?.stop()
            displayLinkHolder = nil
        }
    }

    // MARK: - Red Danger Zone

    private var redDangerZone: some View {
        GeometryReader { geometry in
            let redStartValue: Double = 0.75   // 30 dB GR
            let redEndValue: Double = 1.0      // 40 dB GR (extreme compression)

            Path { path in
                let centerX = geometry.size.width / 2
                let centerY = geometry.size.height / 2
                let pivotX = centerX
                let pivotY = centerY + height * 0.60
                let outerRadius = height * 0.88
                let innerRadius = height * 0.72

                let redStartAngle = startAngle + (redStartValue * (endAngle - startAngle))
                let redEndAngle = startAngle + (redEndValue * (endAngle - startAngle))
                let startRad = redStartAngle * .pi / 180
                let endRad = redEndAngle * .pi / 180

                path.move(to: CGPoint(
                    x: pivotX + outerRadius * cos(startRad),
                    y: pivotY + outerRadius * sin(startRad)
                ))
                path.addArc(
                    center: CGPoint(x: pivotX, y: pivotY),
                    radius: outerRadius,
                    startAngle: .degrees(redStartAngle),
                    endAngle: .degrees(redEndAngle),
                    clockwise: false
                )
                path.addLine(to: CGPoint(
                    x: pivotX + innerRadius * cos(endRad),
                    y: pivotY + innerRadius * sin(endRad)
                ))
                path.addArc(
                    center: CGPoint(x: pivotX, y: pivotY),
                    radius: innerRadius,
                    startAngle: .degrees(redEndAngle),
                    endAngle: .degrees(redStartAngle),
                    clockwise: true
                )
                path.closeSubpath()
            }
            .fill(Color(red: 0.85, green: 0.2, blue: 0.15))
            .opacity(0.7)
        }
    }

    // MARK: - Tick Marks

    private var tickMarks: some View {
        GeometryReader { geometry in
            ForEach(0..<25) { i in
                let fraction = Double(i) / 24.0
                let angle = startAngle + (fraction * (endAngle - startAngle))
                let isMajor = i % 2 == 0
                let centerX = geometry.size.width / 2
                let centerY = geometry.size.height / 2
                let pivotY = centerY + height * 0.60
                let tickRadius = height * 0.80
                let tickLength: CGFloat = isMajor ? 12 : 6
                let tickWidth: CGFloat = isMajor ? 2.0 : 1.2
                let angleRad = angle * .pi / 180
                let tickX = centerX + tickRadius * cos(angleRad)
                let tickY = pivotY + tickRadius * sin(angleRad)

                Rectangle()
                    .fill(Color(red: 0.2, green: 0.2, blue: 0.2))
                    .frame(width: tickWidth, height: tickLength)
                    .rotationEffect(.degrees(angle + 90))
                    .position(x: tickX, y: tickY)
            }
        }
    }

    // MARK: - dB Scale Labels

    private var dbScaleLabels: some View {
        GeometryReader { geometry in
            ForEach(scaleLabels.indices, id: \.self) { index in
                let item = scaleLabels[index]
                let angle = startAngle + (item.value * (endAngle - startAngle))
                let isRed = item.value >= 0.75
                let centerX = geometry.size.width / 2
                let centerY = geometry.size.height / 2
                let pivotY = centerY + height * 0.60
                let labelRadius = height * 0.68
                let angleRad = angle * .pi / 180
                let x = centerX + labelRadius * cos(angleRad)
                let y = pivotY + labelRadius * sin(angleRad)

                Text(item.label)
                    .font(.system(size: height * 0.09, weight: .bold, design: .default))
                    .foregroundColor(isRed ? Color(red: 0.8, green: 0.15, blue: 0.1) : Color(red: 0.2, green: 0.2, blue: 0.2))
                    .position(x: x, y: y)
            }
        }
    }


    // MARK: - Display Link

    /// Polls the DSP value directly on a high-frequency main-thread timer.
    /// Bypasses the AU parameter pipeline so needle latency is one timer tick (~8ms at 120Hz).
    @MainActor
    final class DisplayLinkHolder {
        private var timer: Timer?
        private let valueProvider: () -> Float
        private let onValue: (Double) -> Void
        private var displayed: Double = 0.0
        private var lastDsp: Double = -1.0
        private var frozenTicks: Int = 0

        init(valueProvider: @escaping () -> Float, onValue: @escaping (Double) -> Void) {
            self.valueProvider = valueProvider
            self.onValue = onValue
        }

        func start() {
            timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 120.0, repeats: true) { [weak self] _ in
                guard let self else { return }
                let raw = Double(self.valueProvider())
                let dsp = max(0.0, min(40.0, raw))

                // Track whether the DSP value is changing. When audio stops,
                // process() is no longer called so the DSP value freezes.
                if dsp != self.lastDsp {
                    self.lastDsp = dsp
                    self.frozenTicks = 0
                } else {
                    self.frozenTicks += 1
                }

                // After ~100ms of no change (~12 ticks at 120Hz), consider audio stopped
                // and decay the displayed value to zero.
                let target: Double = self.frozenTicks > 12 ? 0.0 : dsp

                if target > self.displayed {
                    // Attack: snap immediately
                    self.displayed = target
                } else {
                    // Release: smooth decay toward target
                    let decayCoeff = 0.92
                    self.displayed = self.displayed * decayCoeff + target * (1.0 - decayCoeff)
                    if self.displayed < 0.05 { self.displayed = 0.0 }
                }
                self.onValue(self.displayed / 40.0)
            }
        }

        func stop() {
            timer?.invalidate()
            timer = nil
        }
    }

    // MARK: - Needle

    private func needleView(value: Double) -> some View {
        GeometryReader { geometry in
            ZStack {
                let pivotY = height * 0.60
                let needleLength = height * 0.80
                let centerX = geometry.size.width / 2
                let centerY = geometry.size.height / 2

                Rectangle()
                    .fill(
                        LinearGradient(
                            gradient: Gradient(colors: [
                                Color.black,
                                Color(red: 0.2, green: 0.2, blue: 0.2)
                            ]),
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
                    .frame(width: 2, height: needleLength)
                    .position(x: centerX, y: centerY + pivotY - needleLength / 2)
                    .rotationEffect(needleAngle(for: value), anchor: UnitPoint(
                        x: centerX / geometry.size.width,
                        y: (centerY + pivotY) / geometry.size.height
                    ))
                    .shadow(color: .black.opacity(0.5), radius: 2, x: 1, y: 1)

                // Pivot circle
                Circle()
                    .fill(
                        RadialGradient(
                            gradient: Gradient(colors: [
                                Color(red: 0.3, green: 0.3, blue: 0.3),
                                Color.black
                            ]),
                            center: .center,
                            startRadius: 0,
                            endRadius: 4
                        )
                    )
                    .frame(width: 8, height: 8)
                    .overlay(
                        Circle()
                            .stroke(Color.white.opacity(0.3), lineWidth: 1)
                    )
                    .position(x: centerX, y: centerY + pivotY)
                    .shadow(color: .black.opacity(0.3), radius: 1, x: 0, y: 1)
            }
        }
    }
}
