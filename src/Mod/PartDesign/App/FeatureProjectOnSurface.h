// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/FeatureProjectOnSurface.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesign
{

/** PartDesign presentation of Part::ProjectOnSurface.
 *
 * The Part feature owns the complete geometric algorithm: it derives a normal
 * for every planar source, points that normal toward each target face, projects
 * the source, reconstructs wires/faces, and applies offset or height.
 *
 * This subclass deliberately adds no duplicate geometry code. Its constructor
 * only changes property visibility, link scope, and defaults so the inherited
 * feature behaves naturally inside a PartDesign Body and task panel.
 */
class PartDesignExport ProjectOnSurface: public Part::ProjectOnSurface
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::ProjectOnSurface);

public:
    ProjectOnSurface();

    const char* getViewProviderName() const override;
};

}  // namespace PartDesign
