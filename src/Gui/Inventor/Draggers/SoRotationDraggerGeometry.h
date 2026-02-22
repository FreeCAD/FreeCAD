// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
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

#pragma once

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodekits/SoSubKit.h>

#include <FCGlobal.h>

namespace Gui
{

class GuiExport SoRotatorGeometryKit: public SoBaseKit
{
    SO_KIT_HEADER(SoLinearGeometryKit);

public:
    static void initClass();

    SoSFVec3f pivotPosition;
    SoSFVec3f geometryScale;

protected:
    SoRotatorGeometryKit();
    ~SoRotatorGeometryKit() override = default;

private:
    using inherited = SoBaseKit;
};

class GuiExport SoRotatorGeometry: public SoRotatorGeometryKit
{
    SO_KIT_HEADER(SoRotatorGeometry);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(drawStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arcCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(arc);
    SO_KIT_CATALOG_ENTRY_HEADER(pivotSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(rotorPivot);

    SO_KIT_CATALOG_ENTRY_HEADER(_rotorPivotTranslation);

public:
    static void initClass();
    SoRotatorGeometry();

    SoSFFloat arcAngle;  //!< in radians
    SoSFFloat arcRadius;
    SoSFFloat sphereRadius;
    SoSFFloat arcThickness;

protected:
    ~SoRotatorGeometry() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 10;  //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometryKit;
};

class GuiExport SoRotatorGeometry2: public SoRotatorGeometry
{
    SO_KIT_HEADER(SoRotatorGeometry2);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrowSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrowSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrowTransform);
    SO_KIT_CATALOG_ENTRY_HEADER(leftArrow);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrowSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrowSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrowTransform);
    SO_KIT_CATALOG_ENTRY_HEADER(rightArrow);

public:
    static void initClass();
    SoRotatorGeometry2();

    SoSFFloat coneBottomRadius;
    SoSFFloat coneHeight;
    SoSFBool leftArrowVisible;
    SoSFBool rightArrowVisible;

    void toggleArrowVisibility();

protected:
    ~SoRotatorGeometry2() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 10;  //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometry;
};

class GuiExport SoRotatorArrow: public SoRotatorGeometryKit
{
    SO_KIT_HEADER(SoRotatorArrow);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowBody);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowTip);

    SO_KIT_CATALOG_ENTRY_HEADER(_arrowTransform);
    SO_KIT_CATALOG_ENTRY_HEADER(_arrowBodyTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(_arrowTipTranslation);

public:
    static void initClass();
    SoRotatorArrow();

    SoSFFloat coneBottomRadius;
    SoSFFloat coneHeight;
    SoSFFloat cylinderHeight;
    SoSFFloat cylinderRadius;
    SoSFFloat radius;
    SoSFFloat minRadius;

    void flipArrow();

protected:
    ~SoRotatorArrow() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 10;  //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometryKit;
};

class GuiExport SoRotatorGeometryBaseKit: public SoBaseKit
{
    SO_KIT_HEADER(SoRotatorGeometryBaseKit);

public:
    static void initClass();

    SoSFRotation rotation;    //!< set from the parent dragger
    SoSFVec3f geometryScale;  //!< set from the parent dragger
    SoSFBool active;          //!< set from the parent dragger

protected:
    SoRotatorGeometryBaseKit();
    ~SoRotatorGeometryBaseKit() override = default;

private:
    using inherited = SoBaseKit;
};

class GuiExport SoRotatorBase: public SoRotatorGeometryBaseKit
{
    SO_KIT_HEADER(SoArrowBase);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(baseColor);
    SO_KIT_CATALOG_ENTRY_HEADER(drawStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arcCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(arc);

public:
    static void initClass();
    SoRotatorBase();

    SoSFFloat arcRadius;
    SoSFFloat minArcRadius;
    SoSFFloat arcThickness;
    SoSFColor color;

protected:
    ~SoRotatorBase() override = default;

    void notify(SoNotList* notList) override;

private:
    constexpr static int segments = 50;  //!< segments of the arc per arcAngle

    using inherited = SoRotatorGeometryBaseKit;
};

}  // namespace Gui
