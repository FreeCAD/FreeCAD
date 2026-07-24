// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <array>

#include "DrawSketchHandler3D.h"

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
    enum class State
    {
        PickFirst,
        PickSecondPoint,
        PickThirdPoint
    };

    void resetToPickFirst();

    State state {State::PickFirst};
    std::array<Base::Vector3d, 3> threePoints;
};

}  // namespace Sketcher3DGui
