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

#ifndef SKETCHERGUI_SOZOOMTRANSLATION_H
#define SKETCHERGUI_SOZOOMTRANSLATION_H

#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoTransformation.h>
#include <Mod/Sketcher/SketcherGlobal.h>

namespace SketcherGui {

class SketcherGuiExport SoZoomTranslation : public SoTranslation {
    typedef SoTranslation inherited;

    SO_NODE_HEADER(SoZoomTranslation);

public:
    static void initClass();
    SoZoomTranslation();
    SoSFVec3f abPos;

protected:
    virtual ~SoZoomTranslation() {};
    virtual void doAction(SoAction * action);
    virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
    virtual void getMatrix(SoGetMatrixAction * action);
    virtual void GLRender(SoGLRenderAction *action);
    virtual void getBoundingBox(SoGetBoundingBoxAction * action);
    virtual void callback(SoCallbackAction * action);
    virtual void pick(SoPickAction * action);
    float getScaleFactor(SoAction * action) const;
};

}
#endif // SKETCHERGUI_SOZOOMTRANSLATION_H
