#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Sauli Kiviranta
# Compile the Rev 3.1 Phase 1 standalone tests (no Qt, OCCT, or Sketcher).
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../../.." && pwd)"
CXX="${CXX:-g++}"
OUT="${1:-/tmp/sketch-entity-id-phase1}"
"$CXX" -std=c++17 -O2 -Wall -Wextra \
  -DSKETCH_ENTITY_ID_STANDALONE \
  -I "$ROOT/src" \
  "$ROOT/src/Mod/Sketcher/App/SketchEntityId.cpp" \
  "$ROOT/src/Mod/Sketcher/App/SketchEntityIdStandaloneTest.cpp" \
  -o "$OUT"
echo "built $OUT"
