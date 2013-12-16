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

#ifndef SKETCHERGUI_SODATUMLABEL_H
#define SKETCHERGUI_SODATUMLABEL_H

#include <Inventor/fields/SoSubField.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/fields/SoSFImage.h>

namespace SketcherGui {

class SketcherGuiExport SoDatumLabel : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoDatumLabel);

public:
  enum Type
  {
  ANGLE,
  DISTANCE,
  DISTANCEX,
  DISTANCEY,
  RADIUS,
  SYMMETRIC
  };

    static void initClass();
    SoDatumLabel();

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

protected:
    virtual ~SoDatumLabel() {};
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *, SbBox3f &box, SbVec3f &center);
    virtual void generatePrimitives(SoAction * action);
    virtual void notify(SoNotList * l);

private:
    void drawImage();
    SbBox3f bbox;
    float imgWidth;
    float imgHeight;
    bool glimagevalid;
};

}


#endif // SKETCHERGUI_SODATUMLABEL_H
