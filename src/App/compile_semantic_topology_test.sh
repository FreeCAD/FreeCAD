#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Sauli Kiviranta
# Compile the Rev 3.1 Phase 2 + Phase 3 + document-state standalone tests (no Qt, OCCT, or FreeCADApp).
# DESIGN §9 Ambiguous+undo / I5 high-water coverage lives here.
# See docs/toponaming-semantic-ids/WINDOWS-MANUAL.md (SemanticStandalone).
# Optional CMake: -DFREECAD_BUILD_SEMANTIC_STANDALONE_TESTS=ON + ctest -L SemanticStandalone
# Does not affect default Windows pixi run test-debug (-LE Qt).
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
CXX="${CXX:-g++}"
OUT2="${1:-/tmp/semantic-topology-phase2}"
OUT3="${2:-/tmp/semantic-topology-phase3}"
OUTG="${3:-/tmp/semantic-document-state}"
"$CXX" -std=c++17 -O2 -Wall -Wextra \
  -DSEMANTIC_TOPOLOGY_STANDALONE \
  -I "$ROOT/src" -I "$ROOT/src/App" \
  "$ROOT/src/App/SemanticId.cpp" \
  "$ROOT/src/App/SemanticTopology.cpp" \
  "$ROOT/src/App/SemanticReference.cpp" \
  "$ROOT/src/App/SemanticTopologyStandaloneTest.cpp" \
  -o "$OUT2"
echo "built $OUT2"
"$CXX" -std=c++17 -O2 -Wall -Wextra \
  -DSEMANTIC_TOPOLOGY_STANDALONE \
  -I "$ROOT/src" -I "$ROOT/src/App" \
  "$ROOT/src/App/SemanticId.cpp" \
  "$ROOT/src/App/SemanticTopology.cpp" \
  "$ROOT/src/App/SemanticReference.cpp" \
  "$ROOT/src/App/SemanticLinkSub.cpp" \
  "$ROOT/src/App/SemanticLinkSubStandaloneTest.cpp" \
  -o "$OUT3"
echo "built $OUT3"
"$CXX" -std=c++17 -O2 -Wall -Wextra \
  -DSEMANTIC_TOPOLOGY_STANDALONE \
  -I "$ROOT/src" -I "$ROOT/src/App" \
  "$ROOT/src/App/SemanticId.cpp" \
  "$ROOT/src/App/SemanticTopology.cpp" \
  "$ROOT/src/App/SemanticReference.cpp" \
  "$ROOT/src/App/SemanticDocumentState.cpp" \
  "$ROOT/src/App/SemanticDocumentStateStandaloneTest.cpp" \
  -o "$OUTG"
echo "built $OUTG"
