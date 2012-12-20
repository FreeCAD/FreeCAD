/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "ViewProvider.h"
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Gui/Command.h>
//#include <Gui/Document.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProvider,PartGui::ViewProviderPart)

ViewProvider::ViewProvider()
{
}

ViewProvider::~ViewProvider()
{
}

bool ViewProvider::doubleClicked(void)
{
    std::string Msg("Edit ");
    Msg += this->pcObject->Label.getValue();
    Gui::Command::openCommand(Msg.c_str());
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().setEdit('%s',0)",this->pcObject->getNameInDocument());
    return true;
}

void ViewProvider::updateData(const App::Property* prop)
{
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId() && 
        strcmp(prop->getName(),"AddShape") == 0) {
        return;
    }
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId() && 
        strcmp(prop->getName(),"SubShape") == 0) {
        return;
    }
    inherited::updateData(prop);
}
