// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphTypes.h"

#include <string>

namespace CadX
{

struct AdapterClassification
{
    NodeKind nodeKind = NodeKind::UnresolvedDefinition;
    std::string role = "definition";
    std::string containerKind = "none";
    std::string geometryKind = "unavailable";
    std::string provenanceKind = "unknown";
    std::string diagnostic;
};

class AssemblyObjectAdapter
{
public:
    static AdapterClassification classify(const std::string& typeId);
};

}  // namespace CadX
