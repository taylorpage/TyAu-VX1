//
//  ParameterSlider.swift
//  VX1Extension
//
//  Created by Taylor Page on 1/22/26.
//

import SwiftUI

struct ParameterSlider: View {
    @State var param: ObservableAUParameter

    @State private var isDragging = false

    // Fader cap dimensions
    private let thumbWidth: CGFloat = 28
    private let thumbHeight: CGFloat = 44
    // Number of stripe lines on the thumb grip
    private let stripeCount: Int = 9
    // Track is just a thin line
    private let trackHeight: CGFloat = 2
    // Tick marks above/below the track
    private let tickCount: Int = 11

    var normalizedValue: Double {
        Double((param.value - param.min) / (param.max - param.min))
    }

    var body: some View {
        GeometryReader { geo in
            let usableWidth = geo.size.width - thumbWidth
            let thumbX = CGFloat(normalizedValue) * usableWidth

            ZStack(alignment: .leading) {
                // --- Tick marks (top row) ---
                tickMarks(totalWidth: geo.size.width, above: true)

                // --- Tick marks (bottom row) ---
                tickMarks(totalWidth: geo.size.width, above: false)

                // --- Track line ---
                Rectangle()
                    .fill(Color.white.opacity(0.18))
                    .frame(width: geo.size.width, height: trackHeight)
                    .frame(maxHeight: .infinity, alignment: .center)

                // --- Fader cap (thumb) ---
                faderCap
                    .offset(x: thumbX)
                    .frame(maxHeight: .infinity, alignment: .center)
            }
            .contentShape(Rectangle())
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { gesture in
                        if !isDragging {
                            isDragging = true
                            param.onEditingChanged(true)
                        }
                        let clampedX = min(max(gesture.location.x - thumbWidth / 2, 0), usableWidth)
                        let normalized = Float(clampedX / usableWidth)
                        param.value = param.min + normalized * (param.max - param.min)
                    }
                    .onEnded { _ in
                        isDragging = false
                        param.onEditingChanged(false)
                    }
            )
        }
        .frame(height: thumbHeight + 14) // room for tick marks above and below
        .accessibility(identifier: param.displayName)
    }

    // MARK: - Fader Cap

    private var faderCap: some View {
        ZStack {
            // Body with subtle bevel gradient
            RoundedRectangle(cornerRadius: 3)
                .fill(
                    LinearGradient(
                        gradient: Gradient(colors: [
                            Color(white: 0.58),
                            Color(white: 0.38),
                            Color(white: 0.28),
                            Color(white: 0.38)
                        ]),
                        startPoint: .top,
                        endPoint: .bottom
                    )
                )
                .frame(width: thumbWidth, height: thumbHeight)

            // Outer border
            RoundedRectangle(cornerRadius: 3)
                .stroke(Color.black.opacity(0.7), lineWidth: 1)
                .frame(width: thumbWidth, height: thumbHeight)

            // Grip stripes
            gripStripes

            // Center line highlight (the reference white stripe)
            Rectangle()
                .fill(Color.white.opacity(0.85))
                .frame(width: thumbWidth - 4, height: 1.5)
        }
        .shadow(color: .black.opacity(0.55), radius: 4, x: 0, y: 2)
        .frame(width: thumbWidth, height: thumbHeight)
    }

    // MARK: - Grip Stripes

    private var gripStripes: some View {
        let totalGripHeight: CGFloat = CGFloat(stripeCount) * 4.0
        return VStack(spacing: 0) {
            ForEach(0..<stripeCount, id: \.self) { i in
                // Alternate light/dark bands
                Rectangle()
                    .fill(i % 2 == 0 ? Color(white: 0.22) : Color(white: 0.48))
                    .frame(width: thumbWidth - 4, height: 3)
                if i < stripeCount - 1 {
                    Spacer().frame(height: 1)
                }
            }
        }
        .frame(width: thumbWidth - 4, height: totalGripHeight)
        .clipShape(RoundedRectangle(cornerRadius: 2))
    }

    // MARK: - Tick Marks

    private func tickMarks(totalWidth: CGFloat, above: Bool) -> some View {
        let spacing = totalWidth / CGFloat(tickCount - 1)
        return ZStack(alignment: .leading) {
            ForEach(0..<tickCount, id: \.self) { i in
                let isMajor = (i % 5 == 0)
                Rectangle()
                    .fill(Color.white.opacity(isMajor ? 0.45 : 0.22))
                    .frame(width: 1, height: isMajor ? 6 : 4)
                    .offset(
                        x: CGFloat(i) * spacing + thumbWidth / 2 - 0.5,
                        y: above ? -(thumbHeight / 2 + 5) : (thumbHeight / 2 + 5)
                    )
            }
        }
        .frame(maxHeight: .infinity, alignment: .center)
    }
}
