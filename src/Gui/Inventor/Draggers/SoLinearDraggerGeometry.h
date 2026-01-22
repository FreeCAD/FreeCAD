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
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodekits/SoBaseKit.h>

#include <Base/Vector3D.h>

#include <FCGlobal.h>

namespace Gui
{

class GuiExport SoLinearGeometryKit: public SoBaseKit
{
    SO_KIT_HEADER(SoLinearGeometryKit);

public:
    static void initClass();

    SoSFVec3f tipPosition;

protected:
    SoLinearGeometryKit();
    ~SoLinearGeometryKit() override = default;

private:
    using inherited = SoBaseKit;
};

class GuiExport SoArrowGeometry: public SoLinearGeometryKit
{
    SO_KIT_HEADER(SoArrowGeometry);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowBody);
    SO_KIT_CATALOG_ENTRY_HEADER(arrowTip);

    SO_KIT_CATALOG_ENTRY_HEADER(_arrowBodyTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(_arrowTipTranslation);

public:
    static void initClass();
    SoArrowGeometry();

    SoSFFloat coneBottomRadius;
    SoSFFloat coneHeight;
    SoSFFloat cylinderHeight;
    SoSFFloat cylinderRadius;

protected:
    ~SoArrowGeometry() override = default;

    void notify(SoNotList* notList) override;

private:
    using inherited = SoLinearGeometryKit;
};

class GuiExport SoLinearGeometryBaseKit: public SoBaseKit
{
    SO_KIT_HEADER(SoLinearGeometryBaseKit);

public:
    static void initClass();

    SoSFVec3f translation;    //!< set from the parent dragger
    SoSFVec3f geometryScale;  //!< set from the parent dragger
    SoSFBool active;          //!< set from the parent dragger

protected:
    SoLinearGeometryBaseKit();
    ~SoLinearGeometryBaseKit() override = default;

private:
    using inherited = SoBaseKit;
};

class GuiExport SoArrowBase: public SoLinearGeometryBaseKit
{
    SO_KIT_HEADER(SoArrowBase);
    SO_KIT_CATALOG_ENTRY_HEADER(separator);
    SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
    SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(baseColor);
    SO_KIT_CATALOG_ENTRY_HEADER(cylinder);

    SO_KIT_CATALOG_ENTRY_HEADER(_cylinderTranslation);

public:
    static void initClass();
    SoArrowBase();

    SoSFFloat cylinderHeight;
    SoSFFloat cylinderRadius;
    SoSFColor color;

protected:
    ~SoArrowBase() override = default;

    void notify(SoNotList* notList) override;

private:
    using inherited = SoLinearGeometryBaseKit;
};

}  // namespace Gui
