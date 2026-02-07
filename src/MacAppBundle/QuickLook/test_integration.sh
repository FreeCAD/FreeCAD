#!/bin/bash
# SPDX-FileNotice: Part of the FreeCAD project.

# FreeCAD QuickLook Extensions - Integration Test Script
# This script tests the QuickLook extension integration in FreeCAD.app

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Look for FreeCAD.app in common install locations
if [[ -d "${SCRIPT_DIR}/../../../FreeCAD.app" ]]; then
    FREECAD_APP="${SCRIPT_DIR}/../../../FreeCAD.app"
elif [[ -d "${SCRIPT_DIR}/../../../../FreeCAD.app" ]]; then
    FREECAD_APP="${SCRIPT_DIR}/../../../../FreeCAD.app"
else
    # Default to relative path from script
    FREECAD_APP="${SCRIPT_DIR}/../../../FreeCAD.app"
fi

EXTENSIONS_DIR="${FREECAD_APP}/Contents/PlugIns"
THUMBNAIL_EXT="${EXTENSIONS_DIR}/FreeCADThumbnailExtension.appex"
PREVIEW_EXT="${EXTENSIONS_DIR}/FreeCADPreviewExtension.appex"

THUMBNAIL_BUNDLE_ID="org.freecad.FreeCAD.quicklook.thumbnail"
PREVIEW_BUNDLE_ID="org.freecad.FreeCAD.quicklook.preview"

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "OK")
            echo -e "${GREEN}✓${NC} $message"
            ;;
        "FAIL")
            echo -e "${RED}✗${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}⚠${NC} $message"
            ;;
        "INFO")
            echo -e "${BLUE}ℹ${NC} $message"
            ;;
    esac
}

# Main test function
main() {
    echo "FreeCAD QuickLook Extensions - Integration Test"
    echo "=============================================="
    echo "Testing app at: $FREECAD_APP"
    echo

    local total_tests=0
    local passed_tests=0

    # Test 1: Check if FreeCAD.app exists
    ((total_tests++))
    if [[ -d "$FREECAD_APP" ]]; then
        print_status "OK" "FreeCAD.app exists at: $FREECAD_APP"
        ((passed_tests++))
    else
        print_status "FAIL" "FreeCAD.app not found at: $FREECAD_APP"
        print_status "INFO" "Please build and install FreeCAD first with: make install"
        exit 1
    fi

    # Test 2: Check if main FreeCAD executable exists
    ((total_tests++))
    if [[ -f "$FREECAD_APP/Contents/MacOS/FreeCAD" ]]; then
        print_status "OK" "FreeCAD executable exists"
        ((passed_tests++))
    else
        print_status "FAIL" "FreeCAD executable not found"
    fi

    # Test 3: Check if PlugIns directory exists
    ((total_tests++))
    if [[ -d "$EXTENSIONS_DIR" ]]; then
        print_status "OK" "Extensions directory exists: $EXTENSIONS_DIR"
        ((passed_tests++))
    else
        print_status "FAIL" "Extensions directory not found: $EXTENSIONS_DIR"
        print_status "INFO" "QuickLook extensions may not have been built. Check cmake configuration."
    fi

    # Test 4: Check if thumbnail extension exists
    ((total_tests++))
    if [[ -d "$THUMBNAIL_EXT" ]]; then
        print_status "OK" "Thumbnail extension exists"
        ((passed_tests++))
    else
        print_status "FAIL" "Thumbnail extension not found: $THUMBNAIL_EXT"
    fi

    # Test 5: Check if preview extension exists
    ((total_tests++))
    if [[ -d "$PREVIEW_EXT" ]]; then
        print_status "OK" "Preview extension exists"
        ((passed_tests++))
    else
        print_status "FAIL" "Preview extension not found: $PREVIEW_EXT"
    fi

    # Test 6: Check if thumbnail extension executable exists
    ((total_tests++))
    if [[ -f "$THUMBNAIL_EXT/Contents/MacOS/FreeCADThumbnailExtension" ]]; then
        print_status "OK" "Thumbnail extension executable exists"
        ((passed_tests++))
    else
        print_status "FAIL" "Thumbnail extension executable not found"
    fi

    # Test 7: Check if preview extension executable exists
    ((total_tests++))
    if [[ -f "$PREVIEW_EXT/Contents/MacOS/FreeCADPreviewExtension" ]]; then
        print_status "OK" "Preview extension executable exists"
        ((passed_tests++))
    else
        print_status "FAIL" "Preview extension executable not found"
    fi

    # Test 8: Check if thumbnail extension Info.plist exists
    ((total_tests++))
    if [[ -f "$THUMBNAIL_EXT/Contents/Info.plist" ]]; then
        print_status "OK" "Thumbnail extension Info.plist exists"
        ((passed_tests++))
    else
        print_status "FAIL" "Thumbnail extension Info.plist not found"
    fi

    # Test 9: Check if preview extension Info.plist exists
    ((total_tests++))
    if [[ -f "$PREVIEW_EXT/Contents/Info.plist" ]]; then
        print_status "OK" "Preview extension Info.plist exists"
        ((passed_tests++))
    else
        print_status "FAIL" "Preview extension Info.plist not found"
    fi

    # Test 10: Check thumbnail extension bundle ID
    ((total_tests++))
    if [[ -f "$THUMBNAIL_EXT/Contents/Info.plist" ]]; then
        local bundle_id=$(plutil -extract CFBundleIdentifier raw "$THUMBNAIL_EXT/Contents/Info.plist" 2>/dev/null)
        if [[ "$bundle_id" == "$THUMBNAIL_BUNDLE_ID" ]]; then
            print_status "OK" "Thumbnail extension has correct bundle ID: $bundle_id"
            ((passed_tests++))
        else
            print_status "FAIL" "Thumbnail extension bundle ID incorrect: $bundle_id (expected: $THUMBNAIL_BUNDLE_ID)"
        fi
    fi

    # Test 11: Check preview extension bundle ID
    ((total_tests++))
    if [[ -f "$PREVIEW_EXT/Contents/Info.plist" ]]; then
        local bundle_id=$(plutil -extract CFBundleIdentifier raw "$PREVIEW_EXT/Contents/Info.plist" 2>/dev/null)
        if [[ "$bundle_id" == "$PREVIEW_BUNDLE_ID" ]]; then
            print_status "OK" "Preview extension has correct bundle ID: $bundle_id"
            ((passed_tests++))
        else
            print_status "FAIL" "Preview extension bundle ID incorrect: $bundle_id (expected: $PREVIEW_BUNDLE_ID)"
        fi
    fi

    # Test 12: Basic FreeCAD launch test
    ((total_tests++))
    print_status "INFO" "Testing FreeCAD launch (--version)..."
    if timeout 10 "$FREECAD_APP/Contents/MacOS/FreeCAD" --version >/dev/null 2>&1; then
        print_status "OK" "FreeCAD launches successfully"
        ((passed_tests++))
    else
        print_status "FAIL" "FreeCAD failed to launch or crashed"
        print_status "WARN" "This will prevent QuickLook registration from working"
    fi

    # Extension signing tests (optional - don't count toward pass/fail)
    echo
    print_status "INFO" "Code Signing Status:"

    if codesign -v "$THUMBNAIL_EXT" >/dev/null 2>&1; then
        print_status "OK" "Thumbnail extension is signed"
    else
        print_status "WARN" "Thumbnail extension is unsigned (normal for development builds)"
    fi

    if codesign -v "$PREVIEW_EXT" >/dev/null 2>&1; then
        print_status "OK" "Preview extension is signed"
    else
        print_status "WARN" "Preview extension is unsigned (normal for development builds)"
    fi

    # App signing status
    if codesign -v "$FREECAD_APP" >/dev/null 2>&1; then
        print_status "OK" "FreeCAD.app is signed"
    else
        print_status "WARN" "FreeCAD.app is unsigned (normal for development builds)"
    fi

    # Optional tests (don't count toward pass/fail)
    echo
    print_status "INFO" "Additional Information:"

    # Show signing details if available
    if command -v codesign >/dev/null 2>&1; then
        echo
        print_status "INFO" "Signing Details:"

        echo "  FreeCAD.app:"
        codesign -dv "$FREECAD_APP" 2>&1 | grep -E "(Identifier|Authority|Signature)" | head -3 | sed 's/^/    /' || echo "    No signature information"

        echo "  Thumbnail Extension:"
        codesign -dv "$THUMBNAIL_EXT" 2>&1 | grep -E "(Identifier|Authority|Signature)" | head -3 | sed 's/^/    /' || echo "    No signature information"

        echo "  Preview Extension:"
        codesign -dv "$PREVIEW_EXT" 2>&1 | grep -E "(Identifier|Authority|Signature)" | head -3 | sed 's/^/    /' || echo "    No signature information"
    fi

    # Show current registration status (if pluginkit is available)
    if command -v pluginkit >/dev/null 2>&1; then
        echo
        print_status "INFO" "Current Registration Status:"

        if pluginkit -m -v -i "$THUMBNAIL_BUNDLE_ID" >/dev/null 2>&1; then
            print_status "OK" "Thumbnail extension is registered with system"
        else
            print_status "WARN" "Thumbnail extension not registered (normal before first successful FreeCAD launch)"
        fi

        if pluginkit -m -v -i "$PREVIEW_BUNDLE_ID" >/dev/null 2>&1; then
            print_status "OK" "Preview extension is registered with system"
        else
            print_status "WARN" "Preview extension not registered (normal before first successful FreeCAD launch)"
        fi
    fi

    # Check for gatekeeper issues
    echo
    print_status "INFO" "Security Status:"

    if command -v spctl >/dev/null 2>&1; then
        if spctl -a -v "$FREECAD_APP" >/dev/null 2>&1; then
            print_status "OK" "FreeCAD.app passes Gatekeeper checks"
        else
            print_status "WARN" "FreeCAD.app rejected by Gatekeeper (normal for unsigned development builds)"
            print_status "INFO" "You may need to: sudo xattr -rd com.apple.quarantine '$FREECAD_APP'"
        fi
    fi

    # Check for quarantine attributes
    if xattr "$FREECAD_APP" 2>/dev/null | grep -q quarantine; then
        print_status "WARN" "FreeCAD.app has quarantine attributes"
        print_status "INFO" "Remove with: sudo xattr -rd com.apple.quarantine '$FREECAD_APP'"
    else
        print_status "OK" "No quarantine attributes found"
    fi

    # Summary
    echo
    echo "Test Results:"
    echo "============"
    echo "Passed: $passed_tests/$total_tests core tests"

    if [[ $passed_tests -eq $total_tests ]]; then
        print_status "OK" "All core tests passed! QuickLook extensions are properly built and integrated."
        echo
        echo "Next Steps:"
        echo "  1. Ensure FreeCAD launches successfully to trigger extension registration"
        echo "  2. Test QuickLook functionality with .FCStd files in Finder"
        echo "  3. Look for system notification about Quick Look extensions being added"
        return 0
    else
        print_status "FAIL" "Some core tests failed. Please check the build configuration."
        echo
        echo "Troubleshooting:"
        echo "  1. Ensure you're using Unix Makefiles generator: cmake -G 'Unix Makefiles'"
        echo "  2. Check that FREECAD_CREATE_MAC_APP=ON in cmake configuration"
        echo "  3. Run 'make install' to build and install FreeCAD with QuickLook extensions"
        echo "  4. If FreeCAD crashes, try removing quarantine attributes or ad-hoc signing"
        return 1
    fi
}

# Test registration functionality (optional)
test_registration() {
    if [[ "$1" == "--test-registration" ]]; then
        echo
        print_status "INFO" "Testing extension registration..."

        if command -v pluginkit >/dev/null 2>&1; then
            print_status "INFO" "Attempting to register extensions manually..."

            # Try to register extensions
            if pluginkit -a "$THUMBNAIL_EXT" >/dev/null 2>&1; then
                print_status "OK" "Thumbnail extension registration command succeeded"
            else
                print_status "WARN" "Thumbnail registration command failed (may already be registered)"
            fi

            if pluginkit -a "$PREVIEW_EXT" >/dev/null 2>&1; then
                print_status "OK" "Preview extension registration command succeeded"
            else
                print_status "WARN" "Preview registration command failed (may already be registered)"
            fi

            # Try to enable extensions
            pluginkit -e use -i "$THUMBNAIL_BUNDLE_ID" >/dev/null 2>&1 || print_status "WARN" "Thumbnail activation command failed"
            pluginkit -e use -i "$PREVIEW_BUNDLE_ID" >/dev/null 2>&1 || print_status "WARN" "Preview activation command failed"

            sleep 2  # Give system time to process

            # Check final status
            if pluginkit -m -v -i "$THUMBNAIL_BUNDLE_ID" >/dev/null 2>&1; then
                print_status "OK" "Thumbnail extension successfully registered and active"
            else
                print_status "FAIL" "Thumbnail extension registration failed"
            fi

            if pluginkit -m -v -i "$PREVIEW_BUNDLE_ID" >/dev/null 2>&1; then
                print_status "OK" "Preview extension successfully registered and active"
            else
                print_status "FAIL" "Preview extension registration failed"
            fi

            echo
            print_status "INFO" "Try testing with a .FCStd file in Finder now"
        else
            print_status "WARN" "pluginkit not available for registration testing"
        fi
    fi
}

# Show usage information
show_usage() {
    echo "Usage: $0 [--test-registration] [--help]"
    echo
    echo "Options:"
    echo "  --test-registration  Also test extension registration with pluginkit"
    echo "  --help              Show this help message"
    echo
    echo "This script tests the QuickLook extension integration in FreeCAD.app."
    echo "Run this after building and installing FreeCAD: 'make install'"
    echo
    echo "The script will look for FreeCAD.app in common install locations relative to the script."
}

# Handle command line arguments
case "${1:-}" in
    --help)
        show_usage
        exit 0
        ;;
    --test-registration)
        main
        test_registration "$1"
        ;;
    "")
        main
        ;;
    *)
        echo "Unknown option: $1"
        show_usage
        exit 1
        ;;
esac
