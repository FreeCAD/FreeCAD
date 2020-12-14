/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef COIN_SOFCBOUNDINGBOX_H
#define COIN_SOFCBOUNDINGBOX_H

#include <Inventor/SbLinear.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFVec4f.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFBool.h>

namespace Gui {

/**
 * A subclass of SoShape used to create an axis aligned wire frame box based
 * on the minBounds and maxBounds fields.  The class also has a field which
 * can be toggled on or off for displaying text coordinate labels at the
 * vertices of the box.
 * @note Original source are taken from http://www.wheatchex.com/projects/openinventor/bbox/BoundingBox.h and
 * http://www.wheatchex.com/projects/openinventor/bbox/BoundingBox.cpp
 * @date November 12th, 2001
 * @author Josh Grant
 */
class GuiExport SoFCBoundingBox : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(Gui::SoFCBoundingBox);

public:
    static void initClass();
    static void finish();

public:
    SoFCBoundingBox();
    SoSFVec3f minBounds; /**< minimum box coordinates */
    SoSFVec3f maxBounds; /**< maximum box coordinates */
    SoSFBool coordsOn; /**< If true, the coordinates are displayed at each vertex */
    SoSFBool dimensionsOn; /**< If true, the dimensions are displayed in x,y and z direction */


protected:
    virtual ~SoFCBoundingBox();
    virtual void GLRender(SoGLRenderAction *action);
    virtual void generatePrimitives (SoAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);

private:
    SoSeparator        *root, *textSep, *dimSep;
    SoCoordinate3      *bboxCoords;
    SoIndexedLineSet   *bboxLines;
};

/**
 * This is a special group node which must be defined to ignore bounding box actions.
 * @author Werner Mayer
 */
class GuiExport SoSkipBoundingGroup : public SoGroup {
    typedef SoGroup inherited;

    SO_NODE_HEADER(Gui::SoSkipBoundingGroup);

public:
    static void initClass(void);
    static void finish(void);
    SoSkipBoundingGroup(void);

    enum Modes {
        INCLUDE_BBOX, EXCLUDE_BBOX
    };

    SoSFEnum mode;

    virtual void getBoundingBox(SoGetBoundingBoxAction *action);

protected:
    virtual ~SoSkipBoundingGroup();
};

} // namespace Gui


#endif //COIN_SOFCBOUNDINGBOX_H
