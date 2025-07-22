/***************************************************************************
 *   Copyright (c) 2024 Ondsel (PL Boyer) <development@ondsel.com>         *
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
#endif

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "ViewProviderDatum.h"


using namespace PartGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartGui::ViewProviderLine, Gui::ViewProviderLine)

ViewProviderLine::ViewProviderLine()
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
}

bool ViewProviderLine::doubleClicked()
{
    showAttachmentEditor();
    return true;
}

PROPERTY_SOURCE_WITH_EXTENSIONS(PartGui::ViewProviderPlane, Gui::ViewProviderPlane)

ViewProviderPlane::ViewProviderPlane()
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
}

bool ViewProviderPlane::doubleClicked()
{
    showAttachmentEditor();
    return true;
}


PROPERTY_SOURCE_WITH_EXTENSIONS(PartGui::ViewProviderPoint, Gui::ViewProviderPoint)

ViewProviderPoint::ViewProviderPoint()
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
}

bool ViewProviderPoint::doubleClicked()
{
    showAttachmentEditor();
    return true;
}


PROPERTY_SOURCE_WITH_EXTENSIONS(PartGui::ViewProviderLCS, Gui::ViewProviderCoordinateSystem)

ViewProviderLCS::ViewProviderLCS()
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
}

bool ViewProviderLCS::doubleClicked()
{
    showAttachmentEditor();
    return true;
}
