/***************************************************************************
 *   (c) 2011 Luke Parry                                                   *
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

#ifndef GUI_SOAUTOZOOMTRANSLATION_H
#define GUI_SOAUTOZOOMTRANSLATION_H

#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoTransformation.h>

namespace Gui {

class GuiExport SoAutoZoomTranslation : public SoTransformation  {
    typedef SoTransformation  inherited;

    SO_NODE_HEADER(SoAutoZoomTranslation);

public:
    static void initClass();
    SoAutoZoomTranslation();
    //SoSFVec3f abPos;

    SoSFFloat scaleFactor;

protected:
    virtual ~SoAutoZoomTranslation() {};
    virtual void doAction(SoAction * action);
    virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
    virtual void getMatrix(SoGetMatrixAction * action);
    virtual void GLRender(SoGLRenderAction *action);
    virtual void getBoundingBox(SoGetBoundingBoxAction * action);
    virtual void callback(SoCallbackAction * action);
    virtual void pick(SoPickAction * action);
    float getScaleFactor(SoAction*) const;

private:
    //void setAutoScale(void);
};

}
#endif // GUI_SOAUTOZOOMTRANSLATION_H
