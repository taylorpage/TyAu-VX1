//
//  VX1App.swift
//  VX1
//
//  Created by Taylor Page on 1/22/26.
//

import SwiftUI

@main
struct VX1App: App {
    private let hostModel = AudioUnitHostModel()

    var body: some Scene {
        WindowGroup {
            ContentView(hostModel: hostModel)
        }
    }
}
