#!/bin/bash

# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
# SPDX-FileNotice: Part of the FreeCAD project.

# Assemble an experimental Windows FreeCAD bundle from a LibPack-based build.
#
# Unlike package/rattler-build/windows/create_bundle.sh (which sources everything from a conda
# environment), this script sources the redistributable tree from a "cmake --install" staging
# prefix produced by a LibPack build. The LibPack supplies OCCT, Qt6 and Python; FreeCAD supplies
# its own bundled Coin/Pivy. The resulting tree matches the layout the NSIS installer expects
# (bin/ data/ doc/ Ext/ lib/ Mod/).
#
# Required environment:
#   BUILD_TAG         Release tag to attach artifacts to (e.g. weekly-2026.07.22)
#   TARGET_ARCH       "x86_64" or "arm64" (used in the artifact file name)
#   STAGING_DIR       Path to the "cmake --install" prefix (contains bin/, data/, lib/, Mod/, ...)
#   MAKE_INSTALLER    "true" to also build the NSIS installer (x64 only for now)
#   UPLOAD_RELEASE    "true" to upload artifacts to the release with "gh release upload"
# Optional (code signing, all must be present to sign):
#   WINDOWS_SIGN_RELEASE            "1" to attempt Azure Trusted Signing
#   WINDOWS_AZURE_ENDPOINT
#   WINDOWS_AZURE_CERTIFICATE_PROFILE
#   WINDOWS_AZURE_SIGNING_ACCOUNT

set -euo pipefail

: "${BUILD_TAG:?BUILD_TAG must be set}"
: "${TARGET_ARCH:?TARGET_ARCH must be set}"
: "${STAGING_DIR:?STAGING_DIR must be set}"
MAKE_INSTALLER="${MAKE_INSTALLER:-false}"
UPLOAD_RELEASE="${UPLOAD_RELEASE:-false}"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
nsi_dir="${repo_root}/package/WindowsInstaller"

version_name="FreeCAD_${BUILD_TAG}-Windows-${TARGET_ARCH}-experimental"
copy_dir="${version_name}"

echo "################"
echo "version_name:  ${version_name}"
echo "staging_dir:   ${STAGING_DIR}"
echo "################"

# The "cmake --install" prefix already holds the redistributable tree (bin/data/lib/Mod/Ext, plus
# the LibPack runtime DLLs, Python and Qt plugins copied into bin/ by CopyLibpackDirectories.cmake).
# Copy it wholesale, then prune what does not belong in a runtime bundle.
rm -rf "${copy_dir}"
cp -a "${STAGING_DIR}" "${copy_dir}"

# Import libraries and development headers are not needed at runtime.
find "${copy_dir}" -name \*.a -delete
find "${copy_dir}" -name \*.lib -delete
find "${copy_dir}" -name \*.exp -delete
rm -rf "${copy_dir}/include"

# Qt needs to find its plugins relative to the executable directory.
if [ ! -f "${copy_dir}/bin/qt.conf" ]; then
    printf '[Paths]\nPrefix = ..\n' > "${copy_dir}/bin/qt.conf"
fi

# The NSIS installer copies doc/*.* recursively, so the folder must exist and be non-empty.
mkdir -p "${copy_dir}/doc"
if [ -z "$(ls -A "${copy_dir}/doc" 2>/dev/null)" ]; then
    for candidate in LICENSE COPYING LICENSE.txt; do
        if [ -f "${repo_root}/${candidate}" ]; then
            cp "${repo_root}/${candidate}" "${copy_dir}/doc/"
        fi
    done
fi

# Stage the MSVC runtime redistributables. A conda build gets these from the conda env; a LibPack
# build does not bundle them, so gather them from the runner's Visual Studio redist directory. They
# are needed at runtime in bin/ (portable bundle) and are passed to NSIS as FILES_DEPS.
redist_arch="x64"
if [ "${TARGET_ARCH}" = "arm64" ]; then
    redist_arch="arm64"
fi

find_msvc_redist_dir() {
    local vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    local vs_root=""
    if [ -x "${vswhere}" ]; then
        vs_root="$("${vswhere}" -latest -property installationPath 2>/dev/null | tr -d '\r')"
    fi
    local search_roots=()
    if [ -n "${vs_root}" ]; then
        search_roots+=("$(cygpath -u "${vs_root}")/VC/Redist/MSVC")
    fi
    search_roots+=("/c/Program Files/Microsoft Visual Studio/2022"*/*/VC/Redist/MSVC)
    for root in "${search_roots[@]}"; do
        [ -d "${root}" ] || continue
        # Pick the newest CRT folder for the target architecture.
        local crt
        crt="$(ls -d "${root}"/*/"${redist_arch}"/Microsoft.VC*.CRT 2>/dev/null | sort -V | tail -n 1)"
        if [ -n "${crt}" ] && [ -d "${crt}" ]; then
            echo "${crt}"
            return 0
        fi
    done
    return 1
}

msvc_redist_dir="MSVCRedist"
rm -rf "${msvc_redist_dir}"
mkdir -p "${msvc_redist_dir}"
if crt_dir="$(find_msvc_redist_dir)"; then
    echo "Using MSVC runtime from: ${crt_dir}"
    for dll in vcruntime140.dll vcruntime140_1.dll concrt140.dll msvcp140.dll \
               vcamp140.dll vccorlib140.dll vcomp140.dll; do
        if [ -f "${crt_dir}/${dll}" ]; then
            cp "${crt_dir}/${dll}" "${copy_dir}/bin/"
            cp "${crt_dir}/${dll}" "${msvc_redist_dir}/"
        fi
    done
else
    echo "WARNING: could not locate the Visual Studio MSVC redistributable directory."
    echo "         The bundle relies on the runtime DLLs already present in bin/."
fi

# Record what went into the bundle.
{
    echo "FreeCAD experimental LibPack build"
    echo "Build tag:    ${BUILD_TAG}"
    echo "Architecture: ${TARGET_ARCH}"
    if [ -n "${LIBPACK_DIR:-}" ] && [ -f "${LIBPACK_DIR}/FREECAD_LIBPACK_VERSION" ]; then
        echo "LibPack:      $(tr -d '\r' < "${LIBPACK_DIR}/FREECAD_LIBPACK_VERSION")"
    fi
} > "${copy_dir}/packages.txt"

# --- Code signing (Azure Trusted Signing) -----------------------------------------------------
sign_dir="${copy_dir}"
sign_available=0
if [[ "${WINDOWS_SIGN_RELEASE:-0}" == "1" ]]; then
    tenant="$(az account show --query tenantId -o tsv)"
    export AZURE_IDENTITY_DISABLE_WORKLOAD_IDENTITY=true
    export AZURE_IDENTITY_DISABLE_MANAGED_IDENTITY=true
    unset AZURE_IDENTITY_LOGGING_ENABLED || true

    if az account get-access-token --tenant "${tenant}" \
           --scope "https://codesigning.azure.net/.default" >/dev/null 2>&1; then
        sign_available=1
    fi
fi

sign_file() {
    sign code artifact-signing \
        --artifact-signing-endpoint "${WINDOWS_AZURE_ENDPOINT}" \
        --artifact-signing-certificate-profile "${WINDOWS_AZURE_CERTIFICATE_PROFILE}" \
        --artifact-signing-account "${WINDOWS_AZURE_SIGNING_ACCOUNT}" \
        --timestamp-url https://timestamp.acs.microsoft.com \
        --timestamp-digest sha256 \
        "$1" >/dev/null 2>&1
    # Output is silenced because Azure authentication is extremely noisy with misleading Managed
    # Identity "failure" messages that do not affect the real signing result.
}

if [[ "${sign_available}" == "1" ]]; then
    echo "Azure Artifact Signing access confirmed. Signing binaries..."
    shopt -s nullglob
    files=(
        "${sign_dir}"/*.exe
        "${sign_dir}"/bin/*.exe
        "${sign_dir}"/bin/*.dll
        "${sign_dir}"/bin/*.pyd
    )
    total=${#files[@]}
    count=0
    echo "Signing ${total} files"
    for f in "${files[@]}"; do
        count=$((count + 1))
        echo "Signing [${count}/${total}]: ${f}"
        sign_file "${f}"
    done
    signtool verify -pa "${sign_dir}/bin/FreeCAD.exe" || echo "signtool verify failed (continuing)"
    echo "Signing completed."
else
    echo "No Azure Artifact Signing available -- skipping signing."
fi

# --- Smoke tests ------------------------------------------------------------------------------
echo "Running FreeCAD command-line smoke test..."
if ! "${copy_dir}/bin/freecadcmd.exe" --safe-mode --version; then
    echo "FreeCAD command-line smoke test failed; the Windows bundle cannot start."
    exit 1
fi

echo "Running FreeCAD bundled Pivy smoke test..."
if ! "${copy_dir}/bin/freecadcmd.exe" --safe-mode --console \
        "import pivy; from pivy import coin; print(pivy.__file__); print(coin.SoDB.getVersion())"; then
    echo "FreeCAD bundled Pivy smoke test failed; the bundle cannot import the bundled Coin/Pivy runtime."
    exit 1
fi

# --- Portable 7z archive ----------------------------------------------------------------------
7z a -t7z -mx9 -mmt="${NUMBER_OF_PROCESSORS:-4}" "${version_name}.7z" "${version_name}" -bb
sha256sum "${version_name}.7z" > "${version_name}.7z-SHA256.txt"

# Upload the portable bundle right away so it is published even if a later step (e.g. the
# installer) fails. The installer is a best-effort extra on top of the portable artifact.
if [ "${UPLOAD_RELEASE}" == "true" ]; then
    echo "Uploading portable bundle to release ${BUILD_TAG}..."
    gh release upload --clobber "${BUILD_TAG}" \
        "${version_name}.7z" "${version_name}.7z-SHA256.txt"
fi

# --- Installer (x64 only for now) -------------------------------------------------------------
if [ "${MAKE_INSTALLER}" == "true" ]; then
    # The installer uses NSIS advanced logging (LogSet), which requires a log-enabled NSIS build.
    # The workflow provides one via ${MAKENSIS}; fall back to a makensis found on the system.
    makensis_cmd="${MAKENSIS:-}"
    if [ -z "${makensis_cmd}" ]; then
        makensis_cmd="$(command -v makensis || true)"
    fi
    if [ -z "${makensis_cmd}" ]; then
        for candidate in "/c/Program Files (x86)/NSIS/makensis.exe" "/c/Program Files/NSIS/makensis.exe"; do
            if [ -x "${candidate}" ]; then
                makensis_cmd="${candidate}"
                break
            fi
        done
    fi
    if [ -z "${makensis_cmd}" ]; then
        echo "makensis not found; cannot build the installer."
        exit 1
    fi

    files_freecad="$(cygpath -w "$(pwd)")\\${version_name}"
    files_deps="$(cygpath -w "$(pwd)")\\${msvc_redist_dir}"
    "${makensis_cmd}" -V4 \
        -D"ExeFile=${version_name}-installer.exe" \
        -D"FILES_FREECAD=${files_freecad}" \
        -D"FILES_DEPS=${files_deps}" \
        -X'SetCompressor /FINAL lzma' \
        "${nsi_dir}/FreeCAD-installer.nsi"
    mv "${nsi_dir}/${version_name}-installer.exe" .
    echo "Created installer ${version_name}-installer.exe"

    if [[ "${sign_available}" == "1" ]]; then
        echo "Signing the installer..."
        sign_file "${version_name}-installer.exe" \
            || { echo "Signing the installer failed!"; exit 1; }
    else
        echo "No code signing available, leaving the installer unsigned."
    fi
    sha256sum "${version_name}-installer.exe" > "${version_name}-installer.exe-SHA256.txt"

    if [ "${UPLOAD_RELEASE}" == "true" ]; then
        echo "Uploading installer to release ${BUILD_TAG}..."
        gh release upload --clobber "${BUILD_TAG}" \
            "${version_name}-installer.exe" "${version_name}-installer.exe-SHA256.txt"
    fi
fi

echo "Done."
