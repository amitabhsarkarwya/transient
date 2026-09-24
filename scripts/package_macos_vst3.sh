#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
OUTPUT_DIR="${2:-dist}"
VERSION="${3:-0.0.1}"

PLUGIN_BUNDLE="$BUILD_DIR/TransientShaper_artefacts/Release/VST3/Transient Shaper.vst3"
PACKAGE_NAME="Transient-Shaper-VST3-macOS.pkg"
PACKAGE_PATH="$OUTPUT_DIR/$PACKAGE_NAME"
STAGING_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGING_DIR"' EXIT

if [[ ! -d "$PLUGIN_BUNDLE" ]]; then
    echo "VST3 bundle not found: $PLUGIN_BUNDLE" >&2
    exit 1
fi

INSTALL_ROOT="$STAGING_DIR/root"
INSTALL_BUNDLE="$INSTALL_ROOT/Library/Audio/Plug-Ins/VST3/Transient Shaper.vst3"
mkdir -p "$(dirname "$INSTALL_BUNDLE")" "$OUTPUT_DIR"
ditto "$PLUGIN_BUNDLE" "$INSTALL_BUNDLE"

rm -f "$PACKAGE_PATH" "$PACKAGE_PATH.sha256"
pkgbuild \
    --root "$INSTALL_ROOT" \
    --identifier "com.archiedsp.transientshaper.vst3" \
    --version "$VERSION" \
    --install-location "/" \
    --ownership recommended \
    "$PACKAGE_PATH"

(
    cd "$OUTPUT_DIR"
    shasum -a 256 "$PACKAGE_NAME" > "$PACKAGE_NAME.sha256"
)

echo "Created $PACKAGE_PATH"
cat "$OUTPUT_DIR/$PACKAGE_NAME.sha256"
