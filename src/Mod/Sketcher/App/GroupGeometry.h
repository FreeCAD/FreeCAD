// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>
#include <vector>
#include <Mod/Sketcher/SketcherGlobal.h>
#include <Mod/Part/App/Geometry.h>

namespace Sketcher
{
/// Fit a snapshot of geometry to the position, direction and length of a group handle.
SketcherExport std::vector<std::unique_ptr<Part::Geometry>> transformGroupGeometry(
    const std::vector<Part::Geometry*>& geometry,
    const Base::Vector3d& start,
    const Base::Vector3d& end,
    bool height
);
}  // namespace Sketcher
