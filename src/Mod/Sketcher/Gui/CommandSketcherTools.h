
#ifndef SKETCHERGUI_CommandSketcherTools_H
#define SKETCHERGUI_CommandSketcherTools_H

#include <Mod/Sketcher/App/SketchObject.h>

namespace SketcherGui
{

// These functions are declared here to promote code reuse from other modules

/// Scale the sketch around it's origin by a factor
/// and will not abort the current transaction if it fails
void centerScale(Sketcher::SketchObject* Obj, double scale_factor);

}  // namespace SketcherGui
#endif  // SKETCHERGUI_CommandSketcherTools_H
