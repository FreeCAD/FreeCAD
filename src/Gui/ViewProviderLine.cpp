// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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


#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTranslation.h>


#include <Base/Tools.h>
#include <App/Datums.h>
#include <Gui/Utilities.h>
#include <Gui/ViewParams.h>

#include "ViewProviderLine.h"
#include "ViewProviderCoordinateSystem.h"

#include <SoTextLabel.h>


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderLine, Gui::ViewProviderDatum)


ViewProviderLine::ViewProviderLine()
{
    sPixmap = "Std_Axis";

    pLabel = new SoFrameLabel();
    pLabel->textColor.setValue(1.0, 1.0, 1.0);
    pLabel->horAlignment = SoImage::CENTER;
    pLabel->vertAlignment = SoImage::HALF;
    pLabel->border = false;
    pLabel->frame = false;
    pLabel->textUseBaseColor = true;
    pLabel->size = 8;  // NOLINT
}

ViewProviderLine::~ViewProviderLine() = default;

void ViewProviderLine::attach(App::DocumentObject* obj)
{
    ViewProviderDatum::attach(obj);

    // indexes used to create the edges
    static const int32_t lines[4] = {0, 1, -1};

    SoSeparator* sep = getDatumRoot();

    pCoords = new SoCoordinate3();
    sep->addChild(pCoords);

    auto pLines = new SoIndexedLineSet();
    pLines->coordIndex.setNum(3);
    pLines->coordIndex.setValues(0, 3, lines);
    sep->addChild(pLines);

    pLabelTranslation = new SoTranslation();
    sep->addChild(pLabelTranslation);

    auto ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::SHAPE_ON_TOP);
    sep->addChild(ps);

    sep->addChild(pLabel);

    handlers.addDelayedHandler(
        ViewParams::instance()->getHandle(),
        {"DatumLineSize", "DatumScale"},
        [this](const ParameterGrp::handle&) { updateLineSize(); }
    );

    updateLineSize();
}

void ViewProviderLine::updateLineSize()
{
    // Setup label text and line colors
    const char* name = pcObject->getNameInDocument();

    bool noRole = false;
    auto axisRoles = App::LocalCoordinateSystem::AxisRoles;
    if (strncmp(name, axisRoles[0], strlen(axisRoles[0])) == 0) {
        // X-axis: red
        ShapeAppearance.setDiffuseColor(ViewParams::instance()->getAxisXColor());
        pLabel->string.setValue("X");
    }
    else if (strncmp(name, axisRoles[1], strlen(axisRoles[1])) == 0) {
        // Y-axis: green
        ShapeAppearance.setDiffuseColor(ViewParams::instance()->getAxisYColor());
        pLabel->string.setValue("Y");
    }
    else if (strncmp(name, axisRoles[2], strlen(axisRoles[2])) == 0) {
        // Z-axis: blue
        ShapeAppearance.setDiffuseColor(ViewParams::instance()->getAxisZColor());
        pLabel->string.setValue("Z");
    }
    else {
        noRole = true;
    }

    const auto params = ViewParams::instance();
    const float size = params->getDatumLineSize() * Base::fromPercent(params->getDatumScale());

    auto line = getObject<App::Line>();
    Base::Vector3d dir = line->getBaseDirection();
    SbVec3f verts[2];

    if (noRole) {
        verts[0] = Base::convertTo<SbVec3f>(dir * 2 * size);
        verts[1] = SbVec3f(0, 0, 0);
    }
    else {
        verts[0] = Base::convertTo<SbVec3f>(dir * size);
        verts[1] = Base::convertTo<SbVec3f>(dir * 0.2 * size);
    }

    pCoords->point.setNum(2);
    pCoords->point.setValues(0, 2, verts);

    pLabelTranslation->translation.setValue(Base::convertTo<SbVec3f>(dir * 1.2 * size));
}
