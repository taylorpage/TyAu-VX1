#!/bin/bash

# TyAu-VX1 Build Script
# Builds the plugin in Debug configuration and registers it with the system

set -e  # Exit on error

echo "🎸 Building TyAu-VX1 plugin..."

# Build in Debug configuration
xcodebuild -project VX1.xcodeproj \
    -scheme VX1 \
    -configuration Debug \
    build \
    -allowProvisioningUpdates

echo "✅ Build succeeded!"

# Register the Audio Unit extension
echo "📝 Registering Audio Unit extension..."
open /Users/taylorpage/Library/Developer/Xcode/DerivedData/VX1-*/Build/Products/Debug/VX1.app

echo "🎸 VX1 is ready! Load it in Logic Pro."
