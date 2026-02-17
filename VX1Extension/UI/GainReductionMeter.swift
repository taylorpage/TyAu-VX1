//
//  GainReductionMeter.swift
//  VX1Extension
//
//  Gain reduction meter for visual feedback
//

import SwiftUI

struct GainReductionMeter: View {
    @State var param: ObservableAUParameter

    let meterWidth: CGFloat = 280
    let meterHeight: CGFloat = 20
    let maxGainReduction: Float = 30.0  // Maximum GR to display (in dB)

    var body: some View {
        VStack(spacing: 4) {
            // Meter bar
            ZStack(alignment: .leading) {
                // Background with border
                RoundedRectangle(cornerRadius: 4)
                    .stroke(Color.white.opacity(0.3), lineWidth: 1)
                    .background(
                        RoundedRectangle(cornerRadius: 4)
                            .fill(Color.black.opacity(0.7))
                    )
                    .frame(width: meterWidth, height: meterHeight)

                // Meter fill
                let fillWidth = CGFloat(min(param.value / maxGainReduction, 1.0)) * (meterWidth - 4)

                // Gradient fill - green to yellow to red
                LinearGradient(
                    gradient: Gradient(stops: [
                        .init(color: Color(red: 0.2, green: 0.8, blue: 0.3), location: 0.0),
                        .init(color: Color(red: 0.8, green: 0.8, blue: 0.2), location: 0.4),
                        .init(color: Color(red: 0.9, green: 0.5, blue: 0.1), location: 0.7),
                        .init(color: Color(red: 0.9, green: 0.2, blue: 0.2), location: 1.0)
                    ]),
                    startPoint: .leading,
                    endPoint: .trailing
                )
                .frame(width: max(fillWidth, 0), height: meterHeight - 4)
                .cornerRadius(3)
                .padding(.leading, 2)

                // Scale marks overlay
                HStack(spacing: 0) {
                    ForEach([3, 6, 10, 15, 20], id: \.self) { db in
                        let position = CGFloat(db) / CGFloat(maxGainReduction) * meterWidth

                        Rectangle()
                            .fill(Color.white.opacity(0.2))
                            .frame(width: 1, height: meterHeight)
                            .offset(x: position - 1)
                    }
                }
                .frame(width: meterWidth, height: meterHeight, alignment: .leading)
            }

            // Scale labels
            HStack(spacing: 0) {
                Text("0")
                    .font(.system(size: 7, weight: .medium))
                    .foregroundColor(.white.opacity(0.6))
                    .frame(width: 15, alignment: .leading)

                Spacer()

                Text("3")
                    .font(.system(size: 7, weight: .medium))
                    .foregroundColor(.white.opacity(0.6))

                Spacer()

                Text("6")
                    .font(.system(size: 7, weight: .medium))
                    .foregroundColor(.white.opacity(0.6))

                Spacer()

                Text("10")
                    .font(.system(size: 7, weight: .medium))
                    .foregroundColor(.white.opacity(0.6))

                Spacer()

                Text("15")
                    .font(.system(size: 7, weight: .medium))
                    .foregroundColor(.white.opacity(0.6))

                Spacer()

                Text("20")
                    .font(.system(size: 7, weight: .medium))
                    .foregroundColor(.white.opacity(0.6))

                Spacer()

                Text("30")
                    .font(.system(size: 7, weight: .medium))
                    .foregroundColor(.white.opacity(0.6))
                    .frame(width: 15, alignment: .trailing)
            }
            .frame(width: meterWidth)

            // Current value display
            Text(String(format: "%.1f dB", param.value))
                .font(.system(size: 10, weight: .bold))
                .foregroundColor(meterColor)
                .padding(.top, 2)
        }
    }

    // Color based on gain reduction amount
    var meterColor: Color {
        if param.value < 5 {
            return Color(red: 0.2, green: 0.8, blue: 0.3)
        } else if param.value < 15 {
            return Color(red: 0.8, green: 0.8, blue: 0.2)
        } else if param.value < 25 {
            return Color(red: 0.9, green: 0.5, blue: 0.1)
        } else {
            return Color(red: 0.9, green: 0.2, blue: 0.2)
        }
    }
}
