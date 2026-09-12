// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/Gui/ViewProviderExt.h>

namespace SurfaceGui
{
class ViewProviderIntersectionCurve: public PartGui::ViewProviderPartExt
{
    // The shared property macro uses sizeof and type traits for its compile-time checks.
    // NOLINTNEXTLINE(bugprone-sizeof-expression,modernize-type-traits)
    PROPERTY_HEADER_WITH_OVERRIDE(SurfaceGui::ViewProviderIntersectionCurve);

public:
    QIcon getIcon() const override;
};
}  // namespace SurfaceGui
