// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

#ifndef SKETCHERGUI_CommandSketcherTools_H
#define SKETCHERGUI_CommandSketcherTools_H

#include <Mod/Sketcher/App/SketchObject.h>

namespace SketcherGui
{

// These functions are declared here to promote code reuse from other modules

/// Scale the sketch from the current document in edit around it's origin by a factor
/// and will not abort the current transaction if it fails
void centerScale(double scale_factor);

}  // namespace SketcherGui
#endif  // SKETCHERGUI_CommandSketcherTools_H
