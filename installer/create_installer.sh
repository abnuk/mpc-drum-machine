#!/bin/bash
#
# Beatwerk — macOS Installer (.pkg) Builder
#
# Usage:
#   ./installer/create_installer.sh              # uses existing Release build
#   ./installer/create_installer.sh --build       # builds project first
#   ./installer/create_installer.sh --sign "Developer ID Installer: Name (TEAM_ID)"
#
# Output:
#   installer/output/Beatwerk-<version>-macOS.pkg
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
ARTEFACTS_DIR="${BUILD_DIR}/Beatwerk_artefacts/Release"
INSTALLER_DIR="${SCRIPT_DIR}"
OUTPUT_DIR="${INSTALLER_DIR}/output"
STAGING_DIR="${INSTALLER_DIR}/staging"
RESOURCES_DIR="${INSTALLER_DIR}/resources"

APP_NAME="Beatwerk"
IDENTIFIER_BASE="com.beatwerk.app"

# Read version from CMakeLists.txt
VERSION=$(grep -oP '(?<=project\(Beatwerk VERSION )\d+\.\d+\.\d+' "${PROJECT_DIR}/CMakeLists.txt" 2>/dev/null || echo "")
if [ -z "$VERSION" ]; then
    VERSION=$(sed -n 's/.*project(Beatwerk VERSION \([0-9.]*\)).*/\1/p' "${PROJECT_DIR}/CMakeLists.txt")
fi
if [ -z "$VERSION" ]; then
    VERSION="1.0.0"
    echo "Warning: Could not read version from CMakeLists.txt, using ${VERSION}"
fi

# Parse arguments
DO_BUILD=false
SIGN_IDENTITY=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --build)
            DO_BUILD=true
            shift
            ;;
        --sign)
            SIGN_IDENTITY="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--build] [--sign \"Developer ID Installer: Name (TEAM_ID)\"]"
            echo ""
            echo "Options:"
            echo "  --build    Build project in Release mode before creating installer"
            echo "  --sign     Sign the installer package with the given identity"
            echo "  -h         Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "============================================"
echo "  Beatwerk — macOS Installer Builder"
echo "  Version: ${VERSION}"
echo "============================================"
echo ""

# Step 1: Optionally build the project
if [ "$DO_BUILD" = true ]; then
    echo "[1/5] Building project in Release mode..."
    cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" "${PROJECT_DIR}"
    cmake --build "${BUILD_DIR}" --config Release -j "$(sysctl -n hw.ncpu)"
    echo "       Build complete."
else
    echo "[1/5] Skipping build (use --build to compile first)"
fi

# Step 2: Verify artefacts exist
echo "[2/5] Verifying build artefacts..."

STANDALONE_SRC="${ARTEFACTS_DIR}/Standalone/${APP_NAME}.app"
VST3_SRC="${ARTEFACTS_DIR}/VST3/${APP_NAME}.vst3"
AU_SRC="${ARTEFACTS_DIR}/AU/${APP_NAME}.component"

MISSING=0
for artefact in "$STANDALONE_SRC" "$VST3_SRC" "$AU_SRC"; do
    if [ ! -d "$artefact" ]; then
        echo "       ERROR: Missing artefact: ${artefact}"
        MISSING=1
    fi
done

if [ $MISSING -eq 1 ]; then
    echo ""
    echo "Build artefacts not found. Run with --build flag or build manually first:"
    echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
    echo "  cmake --build build --config Release"
    exit 1
fi
echo "       All artefacts found."

# Step 3: Prepare staging areas and build component packages
echo "[3/5] Creating component packages..."

rm -rf "${STAGING_DIR}" "${OUTPUT_DIR}"
mkdir -p "${STAGING_DIR}" "${OUTPUT_DIR}"

# --- Standalone ---
STANDALONE_STAGE="${STAGING_DIR}/standalone"
mkdir -p "${STANDALONE_STAGE}/Applications"
cp -R "$STANDALONE_SRC" "${STANDALONE_STAGE}/Applications/"

pkgbuild \
    --root "${STANDALONE_STAGE}" \
    --identifier "${IDENTIFIER_BASE}.standalone" \
    --version "${VERSION}" \
    --install-location "/" \
    "${OUTPUT_DIR}/Beatwerk-Standalone.pkg" \
    > /dev/null 2>&1

echo "       - Standalone package created"

# --- VST3 ---
VST3_STAGE="${STAGING_DIR}/vst3"
mkdir -p "${VST3_STAGE}/Library/Audio/Plug-Ins/VST3"
cp -R "$VST3_SRC" "${VST3_STAGE}/Library/Audio/Plug-Ins/VST3/"

pkgbuild \
    --root "${VST3_STAGE}" \
    --identifier "${IDENTIFIER_BASE}.vst3" \
    --version "${VERSION}" \
    --install-location "/" \
    "${OUTPUT_DIR}/Beatwerk-VST3.pkg" \
    > /dev/null 2>&1

echo "       - VST3 package created"

# --- Audio Unit ---
AU_STAGE="${STAGING_DIR}/au"
mkdir -p "${AU_STAGE}/Library/Audio/Plug-Ins/Components"
cp -R "$AU_SRC" "${AU_STAGE}/Library/Audio/Plug-Ins/Components/"

pkgbuild \
    --root "${AU_STAGE}" \
    --identifier "${IDENTIFIER_BASE}.au" \
    --version "${VERSION}" \
    --install-location "/" \
    "${OUTPUT_DIR}/Beatwerk-AU.pkg" \
    > /dev/null 2>&1

echo "       - Audio Unit package created"

# Step 4: Build distribution XML with actual sizes
echo "[4/5] Building product installer..."

STANDALONE_SIZE=$(du -sk "${STANDALONE_STAGE}" | cut -f1)
VST3_SIZE=$(du -sk "${VST3_STAGE}" | cut -f1)
AU_SIZE=$(du -sk "${AU_STAGE}" | cut -f1)

# Generate distribution.xml with actual values
DIST_XML="${OUTPUT_DIR}/distribution.xml"
sed \
    -e "s/__VERSION__/${VERSION}/g" \
    -e "s/__STANDALONE_SIZE__/${STANDALONE_SIZE}/g" \
    -e "s/__VST3_SIZE__/${VST3_SIZE}/g" \
    -e "s/__AU_SIZE__/${AU_SIZE}/g" \
    "${INSTALLER_DIR}/distribution.xml" > "${DIST_XML}"

# Generate welcome.html with version
RESOURCES_STAGE="${STAGING_DIR}/resources"
mkdir -p "${RESOURCES_STAGE}"
sed "s/__VERSION__/${VERSION}/g" "${RESOURCES_DIR}/welcome.html" > "${RESOURCES_STAGE}/welcome.html"
cp "${RESOURCES_DIR}/readme.html" "${RESOURCES_STAGE}/readme.html"

# Build the final product archive
INSTALLER_PKG="${OUTPUT_DIR}/Beatwerk-${VERSION}-macOS.pkg"

SIGN_ARGS=()
if [ -n "$SIGN_IDENTITY" ]; then
    SIGN_ARGS=(--sign "$SIGN_IDENTITY")
fi

productbuild \
    --distribution "${DIST_XML}" \
    --package-path "${OUTPUT_DIR}" \
    --resources "${RESOURCES_STAGE}" \
    "${SIGN_ARGS[@]+"${SIGN_ARGS[@]}"}" \
    "${INSTALLER_PKG}" \
    > /dev/null 2>&1

echo "       Product installer created."

# Step 5: Clean up
echo "[5/5] Cleaning up..."

rm -rf "${STAGING_DIR}"
rm -f "${OUTPUT_DIR}/Beatwerk-Standalone.pkg"
rm -f "${OUTPUT_DIR}/Beatwerk-VST3.pkg"
rm -f "${OUTPUT_DIR}/Beatwerk-AU.pkg"
rm -f "${DIST_XML}"

INSTALLER_SIZE=$(du -sh "${INSTALLER_PKG}" | cut -f1)

echo ""
echo "============================================"
echo "  Installer created successfully!"
echo ""
echo "  File: ${INSTALLER_PKG}"
echo "  Size: ${INSTALLER_SIZE}"
echo "============================================"
echo ""
echo "The installer will install:"
echo "  - Standalone App  →  /Applications/${APP_NAME}.app"
echo "  - VST3 Plugin     →  /Library/Audio/Plug-Ins/VST3/${APP_NAME}.vst3"
echo "  - Audio Unit      →  /Library/Audio/Plug-Ins/Components/${APP_NAME}.component"
echo ""

if [ -z "$SIGN_IDENTITY" ]; then
    echo "Note: This installer is unsigned. To sign it for distribution, use:"
    echo "  $0 --sign \"Developer ID Installer: Your Name (TEAM_ID)\""
    echo ""
fi
