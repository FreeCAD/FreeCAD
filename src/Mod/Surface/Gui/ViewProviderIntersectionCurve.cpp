// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Gui/BitmapFactory.h>

#include "ViewProviderIntersectionCurve.h"

using namespace SurfaceGui;

// Static property registration is provided by the shared FreeCAD macro.
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
PROPERTY_SOURCE(SurfaceGui::ViewProviderIntersectionCurve, PartGui::ViewProviderPartExt)

QIcon ViewProviderIntersectionCurve::getIcon() const
{
    return Gui::BitmapFactory().pixmap("Surface_IntersectionCurve");
}
