// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeatureProjectOnSurface.h"

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::ProjectOnSurface, Part::ProjectOnSurface)

ProjectOnSurface::ProjectOnSurface()
{
    // ProjectOnSurface is reference/helper geometry rather than an additive or
    // subtractive feature. Its sources and targets therefore may legitimately
    // live outside the Body that owns this object. Global link scope permits
    // those cross-container references while the normal DAG checks still apply.
    Projection.setScope(App::LinkScope::Global);
    SupportFace.setScope(App::LinkScope::Global);
    SupportFaces.setScope(App::LinkScope::Global);

    // Part keeps SupportFace for compatibility with existing Part documents.
    // The PartDesign UI uses the newer list property because a single operation
    // can project its sources onto several target faces.
    Direction.setStatus(App::Property::Hidden, true);
    SupportFace.setStatus(App::Property::Hidden, true);
    SupportFaces.setStatus(App::Property::Hidden, false);

    // AutoDirection makes Part derive a separate normal for every planar source
    // and orient it toward each target. Keep it visible as an explanation of the
    // feature's behavior, but read-only because PartDesign always uses this mode.
    AutoDirection.setValue(true);
    AutoDirection.setStatus(App::Property::Hidden, false);
    AutoDirection.setStatus(App::Property::ReadOnly, true);

    // "All" retains projected faces where possible and projected edges where a
    // face cannot be reconstructed. It is the most useful PartDesign default.
    Mode.setValue(Part::ProjectOnSurface::AllMode);
}

const char* ProjectOnSurface::getViewProviderName() const
{
    return "PartDesignGui::ViewProviderProjectOnSurface";
}
