// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>
#include <vector>
#include <Mod/Sketcher/SketcherGlobal.h>
#include <Mod/Part/App/Geometry.h>

namespace Sketcher
{
SketcherExport std::vector<std::unique_ptr<Part::Geometry>> transformFixedGroupGeometry(
    const std::vector<Part::Geometry*>& geometry,
    const Base::Vector3d& origin,
    double angle
);

/// Fit a snapshot of geometry to the position, direction and length of a group handle.
SketcherExport std::vector<std::unique_ptr<Part::Geometry>> transformGroupGeometry(
    const std::vector<Part::Geometry*>& geometry,
    const Base::Vector3d& start,
    const Base::Vector3d& end,
    bool height,
    bool useOrigin = false,
    const Base::Vector3d& sourceHandle = Base::Vector3d()
);
}  // namespace Sketcher
