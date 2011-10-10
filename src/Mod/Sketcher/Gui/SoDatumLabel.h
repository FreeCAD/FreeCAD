/***************************************************************************
 *                                                                         *
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
#include <Inventor/SbBox3f.h>
#include <Inventor/fields/SoSFImage.h>

namespace SketcherGui {
 
class SketcherGuiExport SoDatumLabel : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoDatumLabel);

public:

    static void initClass();
    SoDatumLabel();

    SoMFString string;
    SoSFColor  textColor;
    SoSFEnum   justification;
    SoSFName   name;
    SoSFInt32  size;
    SoSFImage  image;

protected:
    virtual ~SoDatumLabel() {};
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *, SbBox3f &box, SbVec3f &center);
    virtual void generatePrimitives(SoAction * action);

private:
    void drawImage();
    float bbx;
    float bby;
};

}


#endif // SKETCHERGUI_SODATUMLABEL_H
