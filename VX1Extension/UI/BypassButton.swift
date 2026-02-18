//
//  BypassButton.swift
//  VX1Extension
//
//  A stomp switch button for audio effect bypass
//

import SwiftUI

struct BypassButton: View {
    @State var param: ObservableAUParameter

    var body: some View {
        Button(action: {
            param.onEditingChanged(true)
            param.boolValue.toggle()
            param.onEditingChanged(false)
        }) {
            Text(param.boolValue ? "BYPASSED" : "BYPASS")
                .font(.system(size: 11, weight: .semibold, design: .monospaced))
                .foregroundColor(param.boolValue ? Color(red: 1.0, green: 0.4, blue: 0.3) : Color.white.opacity(0.5))
                .padding(.horizontal, 20)
                .padding(.vertical, 7)
                .background(
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color.white.opacity(param.boolValue ? 0.08 : 0.04))
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(param.boolValue ? Color(red: 1.0, green: 0.4, blue: 0.3).opacity(0.6) : Color.white.opacity(0.15), lineWidth: 1)
                )
        }
        .buttonStyle(.plain)
        .accessibility(identifier: param.displayName)
    }
}
