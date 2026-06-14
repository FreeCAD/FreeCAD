// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 Werner Mayer <wmayer[at]users.sourceforge.net>
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

#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodes/SoShape.h>
#include <FCGlobal.h>


class SbViewport;
class SoState;
class SbColor;
class SbVec2s;
class SoBaseColor;
class SoCoordinate3;
class SoDrawStyle;
class SoLineSet;
class SoPointSet;
class SoSeparator;
class SoText2;
class SoTranslation;

namespace Gui
{
class GuiExport SoShapeScale: public SoBaseKit
{
    using inherited = SoBaseKit;

    SO_KIT_HEADER(SoShapeScale);

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(scale);
    SO_KIT_CATALOG_ENTRY_HEADER(shape);

public:
    SoShapeScale();
    static void initClass();

    SoSFFloat active;
    SoSFFloat scaleFactor;

protected:
    void GLRender(SoGLRenderAction* action) override;
    ~SoShapeScale() override;
};

class GuiExport SoAxisCrossKit: public SoBaseKit
{
    using inherited = SoBaseKit;

    SO_KIT_HEADER(SoAxisCrossKit);

    SO_KIT_CATALOG_ENTRY_HEADER(xAxis);
    SO_KIT_CATALOG_ENTRY_HEADER(xHead);
    SO_KIT_CATALOG_ENTRY_HEADER(yAxis);
    SO_KIT_CATALOG_ENTRY_HEADER(yHead);
    SO_KIT_CATALOG_ENTRY_HEADER(zAxis);
    SO_KIT_CATALOG_ENTRY_HEADER(zHead);

public:
    SoAxisCrossKit();

    // Overrides default method. All the parts are shapeKits,
    // so this node will not affect the state.
    SbBool affectsState() const override;
    void addWriteReference(SoOutput* out, SbBool isfromfield = false) override;
    void getBoundingBox(SoGetBoundingBoxAction* action) override;

    static void initClass();

private:
    // Constructor calls to build and set up parts.
    void createAxes();
    ~SoAxisCrossKit() override;
};

class GuiExport SoRegPoint: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoRegPoint);

public:
    static void initClass();
    SoRegPoint();

    void notify(SoNotList* node) override;

    SoSFVec3f base;
    SoSFVec3f normal;
    SoSFFloat length;
    SoSFColor color;
    SoSFString text;

protected:
    ~SoRegPoint() override;
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void generatePrimitives(SoAction* action) override;

private:
    SoSeparator* root {nullptr};
    SoSeparator* geometryRoot {nullptr};
    SoBaseColor* geometryColor {nullptr};
    SoCoordinate3* lineCoordinates {nullptr};
    SoLineSet* lineSet {nullptr};
    SoCoordinate3* basePointCoordinates {nullptr};
    SoPointSet* basePointSet {nullptr};
    SoCoordinate3* tipPointCoordinates {nullptr};
    SoPointSet* tipPointSet {nullptr};

    SoSeparator* textRoot {nullptr};
    SoTranslation* move {nullptr};
    SoBaseColor* textColor {nullptr};
    SoText2* label {nullptr};
};

}  // namespace Gui
