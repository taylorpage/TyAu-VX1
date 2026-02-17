//
//  VUMeter.swift
//  VX1Extension
//
//  Vintage analog-style VU meter driven by an ObservableAUParameter.
//  The parameter carries output level in dB (-60 to 0).
//  The VU scale runs from -20 dB (needle far left, value=0.0) to +3 dB (far right, value=1.0),
//  matching the standard VU meter operating point where 0 dBVU = value 0.75.
//

import SwiftUI

struct VUMeter: View {
    @State var param: ObservableAUParameter

    let width: CGFloat
    let height: CGFloat

    // Arc configuration - needle sweeps from left to right over 120°
    private let startAngle: Double = 210
    private let endAngle: Double = 330

    init(param: ObservableAUParameter,
         width: CGFloat = 300,
         height: CGFloat = 180) {
        self.param = param
        self.width = width
        self.height = height
    }

    // Map DSP output level (dB) to VU meter 0.0-1.0 range.
    // VU scale: -20 dB = 0.0, +3 dB = 1.0 (standard VU operating point: 0 dBVU = 0.75)
    private var normalizedValue: Double {
        let db = Double(param.value)
        let clamped = max(-20.0, min(3.0, db))
        return (clamped - (-20.0)) / (3.0 - (-20.0))
    }

    // Needle angle based on a normalized value (0.0 to 1.0)
    private func needleAngle(for value: Double) -> Angle {
        let totalDegrees = endAngle - startAngle
        let degrees = startAngle + (value * totalDegrees)
        return .degrees(degrees + 90)
    }

    // VU meter dB scale labels
    private let scaleLabels = [
        (value: 0.0,   label: "-20"),
        (value: 0.167, label: "-10"),
        (value: 0.25,  label: "-7"),
        (value: 0.375, label: "-5"),
        (value: 0.5,   label: "-3"),
        (value: 0.583, label: "-2"),
        (value: 0.667, label: "-1"),
        (value: 0.75,  label: "0"),
        (value: 0.833, label: "+1"),
        (value: 0.917, label: "+2"),
        (value: 1.0,   label: "+3")
    ]

    // Percentage scale labels (non-linear, matching vintage VU meters)
    private let percentageLabels = [
        (value: 0.0,  label: "0"),
        (value: 0.10, label: "20"),
        (value: 0.22, label: "40"),
        (value: 0.37, label: "60"),
        (value: 0.54, label: "80"),
        (value: 0.75, label: "100")
    ]

    var body: some View {
        // Capture normalizedValue at body level so @Observable tracking fires here
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
                .frame(width: width - 20, height: height - 20)

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
                .frame(width: width - 20, height: height - 20)

            // Meter content
            ZStack {
                redDangerZone
                tickMarks
                dbScaleLabels
                percentageScaleLabels

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
            .frame(width: width - 20, height: height - 20)
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
                .frame(width: width - 20, height: height - 20)
                .blendMode(.overlay)
        }
        .frame(width: width, height: height)
    }

    // MARK: - Red Danger Zone

    private var redDangerZone: some View {
        GeometryReader { geometry in
            let redStartValue: Double = 0.75  // 0 dB
            let redEndValue: Double = 1.0     // +3 dB

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

    // MARK: - Percentage Scale

    private var percentageScaleLabels: some View {
        GeometryReader { geometry in
            ForEach(percentageLabels.indices, id: \.self) { index in
                let item = percentageLabels[index]
                let angle = startAngle + (item.value * (endAngle - startAngle))
                let centerX = geometry.size.width / 2
                let centerY = geometry.size.height / 2
                let pivotY = centerY + height * 0.60
                let labelRadius = height * 0.92
                let angleRad = angle * .pi / 180
                let x = centerX + labelRadius * cos(angleRad)
                let y = pivotY + labelRadius * sin(angleRad)

                Text(item.label)
                    .font(.system(size: height * 0.07, weight: .medium, design: .default))
                    .foregroundColor(Color(red: 0.3, green: 0.3, blue: 0.3))
                    .position(x: x, y: y)
            }

            // "%" symbol next to 100
            let maxItem = percentageLabels.last!
            let angle = startAngle + (maxItem.value * (endAngle - startAngle))
            let centerX = geometry.size.width / 2
            let centerY = geometry.size.height / 2
            let pivotY = centerY + height * 0.60
            let labelRadius = height * 0.92
            let angleRad = angle * .pi / 180
            let x = centerX + labelRadius * cos(angleRad)
            let y = pivotY + labelRadius * sin(angleRad)

            Text("%")
                .font(.system(size: height * 0.06, weight: .medium, design: .default))
                .foregroundColor(Color(red: 0.3, green: 0.3, blue: 0.3))
                .position(x: x + 15, y: y)
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
                    .animation(.linear(duration: 0.05), value: value)

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
