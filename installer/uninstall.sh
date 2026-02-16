#!/bin/bash
#
# MPS Drum Machine — Uninstaller for macOS
#

set -e

APP_NAME="MPS Drum Machine"

STANDALONE_PATH="/Applications/${APP_NAME}.app"
VST3_PATH="/Library/Audio/Plug-Ins/VST3/${APP_NAME}.vst3"
AU_PATH="/Library/Audio/Plug-Ins/Components/${APP_NAME}.component"

echo "=== MPS Drum Machine Uninstaller ==="
echo ""
echo "This will remove the following components (if present):"
echo "  - ${STANDALONE_PATH}"
echo "  - ${VST3_PATH}"
echo "  - ${AU_PATH}"
echo ""
read -p "Do you want to continue? [y/N] " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled."
    exit 0
fi

REMOVED=0

if [ -d "$STANDALONE_PATH" ]; then
    echo "Removing Standalone application..."
    rm -rf "$STANDALONE_PATH"
    REMOVED=$((REMOVED + 1))
fi

if [ -d "$VST3_PATH" ]; then
    echo "Removing VST3 plugin..."
    sudo rm -rf "$VST3_PATH"
    REMOVED=$((REMOVED + 1))
fi

if [ -d "$AU_PATH" ]; then
    echo "Removing Audio Unit plugin..."
    sudo rm -rf "$AU_PATH"
    REMOVED=$((REMOVED + 1))
fi

# Forget installer receipts
pkgutil --pkgs 2>/dev/null | grep "com.mps.drum-machine" | while read -r pkg; do
    echo "Removing package receipt: ${pkg}"
    sudo pkgutil --forget "$pkg" 2>/dev/null || true
done

echo ""
if [ $REMOVED -gt 0 ]; then
    echo "Done. Removed ${REMOVED} component(s)."
    echo "You may need to rescan plugins in your DAW."
else
    echo "No MPS Drum Machine components were found on this system."
fi
