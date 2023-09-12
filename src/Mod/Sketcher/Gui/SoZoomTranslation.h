/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Sketcher/SketcherGlobal.h>


namespace SketcherGui
{

class SketcherGuiExport SoZoomTranslation: public SoTranslation
{
    using inherited = SoTranslation;

    SO_NODE_HEADER(SoZoomTranslation);

public:
    static void initClass();
    SoZoomTranslation();
    SoSFVec3f abPos;
    float getScaleFactor() const
    {
        return scaleFactor;
    }

protected:
    ~SoZoomTranslation() override
    {}
    void doAction(SoAction* action) override;
    void getPrimitiveCount(SoGetPrimitiveCountAction* action) override;
    void getMatrix(SoGetMatrixAction* action) override;
    void GLRender(SoGLRenderAction* action) override;
    void getBoundingBox(SoGetBoundingBoxAction* action) override;
    void callback(SoCallbackAction* action) override;
    void pick(SoPickAction* action) override;
    float calculateScaleFactor(SoAction* action) const;

    mutable float scaleFactor;
};

}  // namespace SketcherGui
#endif  // SKETCHERGUI_SOZOOMTRANSLATION_H
