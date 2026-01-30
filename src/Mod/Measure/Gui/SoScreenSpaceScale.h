// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983[at]gmail.com>          *
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

#ifndef MEASUREGUI_SOSCREENSPACESCALE_H
#define MEASUREGUI_SOSCREENSPACESCALE_H

#include <Inventor/nodes/SoTransformation.h>

#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>

namespace MeasureGui
{

class SoScreenSpaceScale: public SoTransformation
{
    SO_NODE_HEADER(SoScreenSpaceScale);

public:
    static void initClass();
    SoScreenSpaceScale();

    SoSFFloat referenceSize;
    SoSFFloat finalScale;

protected:
    virtual ~SoScreenSpaceScale() = default;
    virtual void GLRender(SoGLRenderAction* action);
    virtual void pick(SoPickAction* action);
    virtual void doAction(SoAction* action);
    virtual void callback(SoCallbackAction* action);
    virtual void getBoundingBox(SoGetBoundingBoxAction* action);
    virtual void getPrimitiveCount(SoGetPrimitiveCountAction* action);

private:
    float calculateScaleFactor(SoAction* action);
};

}  // namespace MeasureGui

#endif  // MEASUREGUI_SOSCREENSPACESCALE_H
