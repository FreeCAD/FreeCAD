// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "DrawSketchHandler3D.h"

class SoTransform;

namespace Sketcher3D
{
class GeomReferencePlane3D;
}

namespace Sketcher3DGui
{

class Sketcher3DGuiExport DrawSketchHandlerReferencePlane3D: public DrawSketchHandler3D
{
public:
    DrawSketchHandlerReferencePlane3D();
    ~DrawSketchHandlerReferencePlane3D() override;

    bool pressButton(const Base::Vector3d& pos) override;
    bool keyPressed(int key) override;

protected:
    void onActivated() override;

private:
    enum class Phase
    {
        None,
        HaveLine,
        CollectPoints
    };

    void reset();
    void commitPlane(std::unique_ptr<Sketcher3D::GeomReferencePlane3D> plane, const Base::Vector3d& base);

    Phase phase {Phase::None};
    int nPoints {0};
    Base::Vector3d points[3];

    SoSwitch* arrowSwitch {nullptr};
    SoTransform* arrowTransform {nullptr};
};

}  // namespace Sketcher3DGui
