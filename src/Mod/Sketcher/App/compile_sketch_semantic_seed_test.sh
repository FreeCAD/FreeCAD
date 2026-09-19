#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Sauli Kiviranta
# Compile Rev 3.1 region + Vertex seed tests (no Qt, OCCT, or Sketcher).
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../../.." && pwd)"
CXX="${CXX:-g++}"
OUT="${1:-/tmp/sketch-semantic-seed-phase1}"
"$CXX" -std=c++17 -O2 -Wall -Wextra \
  -DSKETCH_ENTITY_ID_STANDALONE \
  -DSEMANTIC_TOPOLOGY_STANDALONE \
  -I "$ROOT/src" -I "$ROOT/src/App" \
  "$ROOT/src/App/SemanticId.cpp" \
  "$ROOT/src/App/SemanticTopology.cpp" \
  "$ROOT/src/Mod/Sketcher/App/SketchEntityId.cpp" \
  "$ROOT/src/Mod/Sketcher/App/SketchSemanticSeed.cpp" \
  "$ROOT/src/Mod/Sketcher/App/SketchSemanticSeedStandaloneTest.cpp" \
  -o "$OUT"
echo "built $OUT"
