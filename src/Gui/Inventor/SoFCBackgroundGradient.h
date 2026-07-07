// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2005 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <cstdint>

#include <Inventor/SbColor.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/nodes/SoSubNode.h>
#include <FCGlobal.h>

#include "SoFCScreenSpaceGroup.h"


class SoAction;
class SbColor;
class SoFaceSet;
class SoGLRenderAction;
class SoSeparator;
class SoSwitch;
class SoVertexProperty;

namespace Gui
{

using SoFCScreenSpaceGroup = Inventor::SoFCScreenSpaceGroup;

class GuiExport SoFCBackgroundGradient: public SoFCScreenSpaceGroup
{
    using inherited = SoFCScreenSpaceGroup;

    SO_NODE_HEADER(Gui::SoFCBackgroundGradient);

public:
    enum Gradient
    {
        LINEAR = 0,
        RADIAL = 1
    };
    static void initClass();
    static void finish();
    SoFCBackgroundGradient();

    SoSFEnum gradientMode;
    SoSFColor fromColor;
    SoSFColor toColor;
    SoSFColor midColor;
    SoSFBool useMidColor;

    void setGradient(Gradient grad);
    Gradient getGradient() const;
    void setColorGradient(const SbColor& fromColor, const SbColor& toColor);
    void setColorGradient(const SbColor& fromColor, const SbColor& toColor, const SbColor& midColor);

private:
    struct GeometryState
    {
        Gradient gradient {LINEAR};
        uint32_t fromColor {0};
        uint32_t toColor {0};
        uint32_t midColor {0};
        bool useMidColor {true};
    };

    void ensureGeometry();
    GeometryState currentGeometryState() const;
    void updateLinearGeometry(const GeometryState& state);
    void updateRadialGeometry(const GeometryState& state);

    static constexpr int CircleSegments = 32;

    bool geometryInitialized {false};
    GeometryState appliedGeometryState;

    SoSwitch* gradientSwitch {nullptr};
    SoSeparator* linearSeparator {nullptr};
    SoFaceSet* linearFaces {nullptr};
    SoVertexProperty* linearVertexProperty {nullptr};

    SoSeparator* radialSeparator {nullptr};
    SoFaceSet* radialFan {nullptr};
    SoVertexProperty* radialFanVertexProperty {nullptr};
    SoFaceSet* radialRing {nullptr};
    SoVertexProperty* radialRingVertexProperty {nullptr};

protected:
    ~SoFCBackgroundGradient() override;
    void doAction(SoAction* action) override;
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void GLRenderInPath(SoGLRenderAction* action) override;
};

}  // namespace Gui
