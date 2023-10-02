/***************************************************************************
 *   Copyright (c) 2011-2012 Luke Parry <l.parry@warwick.ac.uk>            *
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

#ifndef GUI_SODATUMLABEL_H
#define GUI_SODATUMLABEL_H

#include <Inventor/SbBox3f.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/nodes/SoShape.h>

#include <FCGlobal.h>


namespace Gui {

class GuiExport SoDatumLabel : public SoShape {
    using inherited = SoShape;

    SO_NODE_HEADER(SoDatumLabel);

public:
    enum Type
    {
        ANGLE,
        DISTANCE,
        DISTANCEX,
        DISTANCEY,
        RADIUS,
        DIAMETER,
        SYMMETRIC
    };

    static void initClass();
    SoDatumLabel();

    /*The points have to be on XY plane, ie they need to be 2D points.
    To draw on other planes, you need to attach a SoTransform to the SoDatumLabel (or parent).*/
    void setPoints(SbVec3f p1, SbVec3f p2);

    SoMFString string;
    SoSFColor  textColor;
    SoSFEnum   datumtype;
    SoSFName   name;
    SoSFInt32  size;
    SoSFFloat  param1;
    SoSFFloat  param2;
    SoSFFloat  param3;
    SoMFVec3f  pnts;
    SoSFVec3f  norm;
    SoSFImage  image;
    SoSFFloat  lineWidth;
    bool       useAntialiasing;
    SbVec3f textOffset;

protected:
    ~SoDatumLabel() override = default;
    void GLRender(SoGLRenderAction *action) override;
    void computeBBox(SoAction *, SbBox3f &box, SbVec3f &center) override;
    void generatePrimitives(SoAction * action) override;
    void notify(SoNotList * l) override;

private:
    float getScaleFactor(SoState*) const;
    void generateDistancePrimitives(SoAction * action, const SbVec3f&, const SbVec3f&);
    void generateDiameterPrimitives(SoAction * action, const SbVec3f&, const SbVec3f&);
    void generateAnglePrimitives(SoAction * action, const SbVec3f&);
    void generateSymmetricPrimitives(SoAction * action, const SbVec3f&, const SbVec3f&);

private:
    void drawImage();
    float imgWidth;
    float imgHeight;
    bool glimagevalid;
};

}


#endif // GUI_SODATUMLABEL_H
