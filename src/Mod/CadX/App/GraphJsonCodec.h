// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphSnapshot.h"

#include <memory>
#include <string>

namespace CadX
{

struct GraphDecodeResult
{
    std::shared_ptr<GraphSnapshot> snapshot;
    std::string errorCode;
    std::string diagnostic;

    explicit operator bool() const noexcept
    {
        return snapshot != nullptr && errorCode.empty();
    }
};

// Lossless, bounded evidence format for a finalized graph.  This is not the
// provider result format: it contains the complete graph so a captured graph
// can be audited, reconstructed, and compared without FreeCAD running.
class GraphJsonCodec
{
public:
    static constexpr const char* schemaVersion = "cadx.assembly-graph-snapshot.v1";

    static std::string encode(const GraphSnapshot& snapshot);
    static GraphDecodeResult decode(const std::string& json);
    static GraphDecodeResult roundTrip(const GraphSnapshot& snapshot);
};

}  // namespace CadX
