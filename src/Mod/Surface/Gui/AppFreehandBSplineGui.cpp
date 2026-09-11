// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ViewProviderFreehandBSpline.h"

void CreateSurfaceFreehandBSplineCommands();

void initSurfaceFreehandBSplineGui()
{
    SurfaceGui::ViewProviderFreehandBSpline::init();
    CreateSurfaceFreehandBSplineCommands();
}
