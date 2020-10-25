/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Python.h>
#endif

#include <Gui/Application.h>
#include <Gui/Control.h>
#include "DlgPrimitives.h"
#include "ViewProviderHelixParametric.h"

using namespace PartGui;


PROPERTY_SOURCE(PartGui::ViewProviderHelixParametric, PartGui::ViewProviderSpline)


ViewProviderHelixParametric::ViewProviderHelixParametric()
{
    sPixmap = "Part_Helix_Parametric";
}

ViewProviderHelixParametric::~ViewProviderHelixParametric()
{

}

std::vector<std::string> ViewProviderHelixParametric::getDisplayModes(void) const
{
    // add your own modes
    std::vector<std::string> StrList;
    StrList.push_back("Wireframe");
    StrList.push_back("Points");

    return StrList;
}

void ViewProviderHelixParametric::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit helix"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    ViewProviderSpline::setupContextMenu(menu, receiver, member);
}

void ViewProviderHelixParametric::updateData(const App::Property* prop)
{
    PartGui::ViewProviderSpline::updateData(prop);
}

bool ViewProviderHelixParametric::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        if (Gui::Control().activeDialog())
            return false;
        auto ObjectName = getObject()->getNameInDocument();
        PartGui::TaskPrimitivesEdit* dlg
            = new PartGui::TaskPrimitivesEdit(PartGui::DlgPrimitives::PrimitiveType::Helix, ObjectName);
        Gui::Control().showDialog(dlg);
        return true;
    }
    else {
        ViewProviderSpline::setEdit(ModNum);
        return true;
    }
}

void ViewProviderHelixParametric::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderSpline::unsetEdit(ModNum);
    }
}

// ------------------------------------------------------------------

PROPERTY_SOURCE(PartGui::ViewProviderSpiralParametric, PartGui::ViewProviderSpline)


ViewProviderSpiralParametric::ViewProviderSpiralParametric()
{
    sPixmap = "Part_Spiral_Parametric";
}

ViewProviderSpiralParametric::~ViewProviderSpiralParametric()
{

}

std::vector<std::string> ViewProviderSpiralParametric::getDisplayModes(void) const
{
    // add your own modes
    std::vector<std::string> StrList;
    StrList.push_back("Wireframe");
    StrList.push_back("Points");

    return StrList;
}
