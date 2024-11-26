/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoAsciiText.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include <App/Datums.h>
#include <Gui/ViewParams.h>

#include "ViewProviderLine.h"
#include "ViewProviderCoordinateSystem.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderLine, Gui::ViewProviderDatum)


ViewProviderLine::ViewProviderLine()
{
    sPixmap = "Std_Axis";
}

ViewProviderLine::~ViewProviderLine() = default;

void ViewProviderLine::attach(App::DocumentObject *obj) {
    ViewProviderDatum::attach(obj);

    // Setup label text and line colors
    const char* name = pcObject->getNameInDocument();

    bool noRole = false;
    auto axisRoles = App::LocalCoordinateSystem::AxisRoles;
    if (strncmp(name, axisRoles[0], strlen(axisRoles[0])) == 0) {
        // X-axis: red
        ShapeAppearance.setDiffuseColor(ViewParams::instance()->getAxisXColor());
        pLabel->string.setValue(SbString("X"));
    }
    else if (strncmp(name, axisRoles[1], strlen(axisRoles[1])) == 0) {
        // Y-axis: green
        ShapeAppearance.setDiffuseColor(ViewParams::instance()->getAxisYColor());
        pLabel->string.setValue(SbString("Y"));
    }
    else if (strncmp(name, axisRoles[2], strlen(axisRoles[2])) == 0) {
        // Z-axis: blue
        ShapeAppearance.setDiffuseColor(ViewParams::instance()->getAxisZColor());
        pLabel->string.setValue(SbString("Z"));
    }
    else {
        noRole = true;
    }

    static const float size = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")->GetFloat("DatumLineSize", 70.0);

    SbVec3f verts[2];
    if (noRole) {
        verts[0] = SbVec3f(0, 0, 2 * size);
        verts[1] = SbVec3f(0, 0, 0);
    }
    else {
        verts[0] = SbVec3f(0, 0, size);
        verts[1] = SbVec3f(0, 0, 0.2 * size);
    }

    // indexes used to create the edges
    static const int32_t lines[4] = { 0, 1, -1 };

    SoSeparator *sep = getDatumRoot();

    auto pCoords = new SoCoordinate3 ();
    pCoords->point.setNum (2);
    pCoords->point.setValues ( 0, 2, verts );
    sep->addChild ( pCoords );

    auto pLines  = new SoIndexedLineSet ();
    pLines->coordIndex.setNum(3);
    pLines->coordIndex.setValues(0, 3, lines);
    sep->addChild ( pLines );

    auto textTranslation = new SoTranslation ();
    textTranslation->translation.setValue(SbVec3f(0, 0, size * 1.1));
    sep->addChild ( textTranslation );

    auto ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::SHAPE_ON_TOP);
    sep->addChild(ps);

    sep->addChild ( getLabel () );
}
