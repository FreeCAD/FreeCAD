/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QPixmap>
#endif

#include <App/Part.h>
#include <App/Document.h>

#include "ActiveObjectList.h"
#include "BitmapFactory.h"
#include "Command.h"

#include "ViewProviderPart.h"
#include "Application.h"
#include "MDIView.h"


using namespace Gui;


PROPERTY_SOURCE_WITH_EXTENSIONS(Gui::ViewProviderPart, Gui::ViewProviderDragger)


/**
 * Creates the view provider for an object group.
 */
ViewProviderPart::ViewProviderPart()
{ 
    initExtension(this);

    sPixmap = "Geofeaturegroup.svg";
}

ViewProviderPart::~ViewProviderPart()
{ }

/**
 * TODO
 * Whenever a property of the group gets changed then the same property of all
 * associated view providers of the objects of the object group get changed as well.
 */
void ViewProviderPart::onChanged(const App::Property* prop) {
    ViewProviderDragger::onChanged(prop);
}

bool ViewProviderPart::doubleClicked(void)
{
    //make the part the active one

    //first, check if the part is already active.
    App::DocumentObject* activePart = nullptr;
    MDIView* activeView = this->getActiveView();
    if ( activeView ) {
        activePart = activeView->getActiveObject<App::DocumentObject*> (PARTKEY);
    }

    if (activePart == this->getObject()){
        //active part double-clicked. Deactivate.
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.getDocument('%s').ActiveView.setActiveObject('%s', None)",
                this->getObject()->getDocument()->getName(),
                PARTKEY);
    } else {
        //set new active part
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.getDocument('%s').ActiveView.setActiveObject('%s', App.getDocument('%s').getObject('%s'))",
                this->getObject()->getDocument()->getName(),
                PARTKEY,
                this->getObject()->getDocument()->getName(),
                this->getObject()->getNameInDocument());
    }

    return true;
}

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPartPython, Gui::ViewProviderPart)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderPart>;
}
