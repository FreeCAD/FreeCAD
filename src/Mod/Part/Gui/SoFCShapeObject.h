/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTGUI_SOFCSHAPEOBJECT_H
#define PARTGUI_SOFCSHAPEOBJECT_H

#include <Inventor/fields/SoSFUInt32.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSubField.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/elements/SoReplacedElement.h>
#include <Mod/Part/PartGlobal.h>

namespace PartGui {

class PartGuiExport SoFCControlPoints : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoFCControlPoints);

public:
    static void initClass();
    SoFCControlPoints();

    SoSFUInt32 numPolesU;
    SoSFUInt32 numPolesV;
    SoSFUInt32 numKnotsU;
    SoSFUInt32 numKnotsV;
    SoSFColor  lineColor;

protected:
    virtual ~SoFCControlPoints() {};
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
    virtual void generatePrimitives(SoAction *action);

private:
    void drawControlPoints(const SbVec3f *,int32_t) const;
};

} // namespace PartGui


#endif // PARTGUI_SOFCSHAPEOBJECT_H

