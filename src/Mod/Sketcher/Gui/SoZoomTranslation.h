// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Werner Mayer
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

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
