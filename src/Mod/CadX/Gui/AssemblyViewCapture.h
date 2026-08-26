// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "../App/AssemblyCapture.h"

#include <string>

namespace CadX
{

struct AssemblyViewState
{
    std::string viewId;
    std::string projection;
    std::string cameraState;
    std::string presentationChecksum;
};

struct AssemblyViewCaptureOptions
{
    // The shared semantic capture currently exposes metadata and primitive
    // dimensions, not tessellated geometry. Both supported detail values are
    // therefore represented by the same bounded semantic graph.
    std::string geometryDetail = "summary";
    bool includeViewState = true;
};

struct AssemblyViewCaptureResult
{
    bool ok = false;
    std::string errorCode;
    std::string diagnostic;
    AssemblyCapture capture;
    AssemblyViewState viewState;
};

class AssemblyViewCapture
{
public:
    AssemblyViewCaptureResult capture(const AssemblyViewCaptureOptions& options = {}) const;
};

}  // namespace CadX
