// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <App/Application.h>

namespace Sketcher
{
// Preferences are copied when creating a layer, never applied to existing layers.
inline ParameterGrp::handle layerDefaults()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/LayerDefaults"
    );
}
}  // namespace Sketcher
