// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Base/Vector3D.h>
#include <Mod/Part/App/FeatureProjectOnSurface.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesign
{

/** PartDesign policy layer over Part::ProjectOnSurface.
 *
 * Part continues to own the projection, wire healing, pcurve reconstruction,
 * face construction, offset and extrusion algorithms. This subclass only
 * calculates the shared projection direction from the planar inputs before
 * delegating to Part::ProjectOnSurface::execute().
 *
 * The upstream Part API has one Direction and one SupportFace, so multiple
 * source elements are accepted only when their plane normals are parallel.
 */
class PartDesignExport ProjectOnSurface: public Part::ProjectOnSurface
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::ProjectOnSurface);

public:
    ProjectOnSurface();

    App::DocumentObjectExecReturn* execute() override;
    const char* getViewProviderName() const override;

private:
    Base::Vector3d calculateDirection() const;
};

}  // namespace PartDesign
