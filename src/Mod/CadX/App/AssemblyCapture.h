// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "GraphTypes.h"

#include <string>
#include <vector>

namespace CadX
{

// This DTO is the only input accepted by the graph builder.  It owns copies
// of capture data and therefore cannot retain DocumentObject/ViewProvider
// pointers after the main-thread capture has completed.
struct AssemblyCapture
{
    std::string documentUid;
    std::string documentName;
    std::string activeAssemblyObjectName;
    std::string activeAssemblyLabel;
    std::string activeAssemblyNodeId;
    std::string activeViewId;
    std::string cameraState;
    bool documentBusy = false;
    bool startGuardMatches = true;
    bool endGuardMatches = true;
    std::vector<NodeRecord> nodes;
    std::vector<EdgeRecord> edges;
    std::vector<std::string> diagnostics;
};

}  // namespace CadX
