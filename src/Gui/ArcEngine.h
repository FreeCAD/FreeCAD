/***************************************************************************
 *   Copyright (c) 2013 Thomas Anderson <blobfish[at]gmx.com>              *
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

#pragma once

#include <FCGlobal.h>
#include <Inventor/fields/SoMFColor.h>
#include <Inventor/engines/SoEngineOutput.h>

#include <Inventor/engines/SoSubEngine.h>
#include <Inventor/engines/SoEngine.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodekits/SoSeparatorKit.h>

class SoText2;
class SoTranslation;
class SoCoordinate3;
class SoIndexedLineSet;

namespace Gui
{


// /*used for generating points for arc display*/
class GuiExport ArcEngine: public SoEngine
{
    SO_ENGINE_HEADER(ArcEngine);

public:
    ArcEngine();
    static void initClass();

    SoSFFloat radius;
    SoSFFloat angle;
    SoSFFloat deviation;

    SoEngineOutput points;
    SoEngineOutput pointCount;
    SoEngineOutput midpoint;

protected:
    void evaluate() override;

private:
    ~ArcEngine() override
    {}
    void defaultValues();  // some non error values if something goes wrong.
};

}  // namespace Gui
