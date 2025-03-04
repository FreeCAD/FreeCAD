// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef SOFCPLACEMENTINDICATORKIT_H
#define SOFCPLACEMENTINDICATORKIT_H

#include <Inventor/nodekits/SoBaseKit.h>

#include <FCGlobal.h>

namespace Gui {

class GuiExport SoFCPlacementIndicatorKit : public SoBaseKit {
    using inherited = SoBaseKit;

    static constexpr double planeIndicatorRadius = 0.4f;
    static constexpr double planeIndicatorMargin = 0.2f;
    static constexpr double planeIndicatorTransparency = 0.3f;

    static constexpr double axisMargin = 0.0f;
    static constexpr double axisLengthDefault = 0.6f;
    static constexpr double axisThickness = 0.065f;

    static constexpr double arrowHeadRadius = axisThickness * 1.25f;
    static constexpr double arrowHeadHeight = arrowHeadRadius * 3.f;

    static constexpr double labelOffset = 0.4f;
    static constexpr int labelFontSize = 9;

    static constexpr double scaleFactorDefault = 40.f;

    SO_KIT_HEADER(SoFCPlacementIndicatorKit);

    SO_KIT_CATALOG_ENTRY_HEADER(root);

public:
    SoFCPlacementIndicatorKit();
    static void initClass();

    void notify(SoNotList* l) override;

    // clang-format off
    enum Part
    {
        Axes           = 1 << 0,
        PlaneIndicator = 1 << 1,
        Labels         = 1 << 2,
        ArrowHeads     = 1 << 3,

        // common configurations
        AllParts       = Axes | PlaneIndicator | Labels | ArrowHeads,
        AxisCross      = Axes | Labels | ArrowHeads
    };

    enum Axis
    {
        X       = 1 << 0,
        Y       = 1 << 1,
        Z       = 1 << 2,
        AllAxes = X | Y | Z
    };
    // clang-format on

    SoSFEnum parts;
    SoSFEnum axes;
    SoSFBool coloredAxis;
    SoSFFloat scaleFactor;
    SoSFFloat axisLength;

private:
    void recomputeGeometry();

    SoSeparator* createOriginIndicator();
    SoSeparator* createGeometry();
    SoSeparator* createAxes();
    SoSeparator* createPlaneIndicator();

    ~SoFCPlacementIndicatorKit() override;
};

} // Gui

#endif //SOFCPLACEMENTINDICATORKIT_H
