/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef GUI_SOSHAPESCALE_H
#define GUI_SOSHAPESCALE_H

#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFVec3f.h>

class SbViewport;
class SoState;
class SbColor;
class SbVec2s;

namespace Gui {
class GuiExport SoShapeScale : public SoBaseKit {
    typedef SoBaseKit inherited;

    SO_KIT_HEADER(SoShapeScale);

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(scale);
    SO_KIT_CATALOG_ENTRY_HEADER(shape);

public:
    SoShapeScale(void);
    static void initClass(void);

    SoSFFloat active;
    SoSFFloat scaleFactor;

protected:
    virtual void GLRender(SoGLRenderAction * action);
    virtual ~SoShapeScale();
};

class GuiExport SoAxisCrossKit : public SoBaseKit {
    typedef SoBaseKit inherited;

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
    virtual SbBool affectsState() const;
    virtual void addWriteReference(SoOutput * out, SbBool isfromfield = FALSE);
    virtual void getBoundingBox(SoGetBoundingBoxAction * action);

    static void initClass();

private:
    // Constructor calls to build and set up parts.
    void createAxes();
    virtual ~SoAxisCrossKit();
};

class GuiExport SoRegPoint : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoRegPoint);

public:
    static void initClass();
    SoRegPoint();

    void notify(SoNotList * node);

    SoSFVec3f base;
    SoSFVec3f normal;
    SoSFFloat length;
    SoSFColor color;
    SoSFString text;

protected:
    virtual ~SoRegPoint();
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
    virtual void generatePrimitives(SoAction *action);

private:
    SoSeparator* root;
};

} // namespace Gui

#endif // GUI_SOSHAPESCALE_H
