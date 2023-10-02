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
# include <QMenu>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>

#include "ViewProviderPart.h"
#include "ActionFunction.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
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
    aPixmap = "Geoassembly.svg";
}

ViewProviderPart::~ViewProviderPart() = default;

/**
 * TODO
 * Whenever a property of the group gets changed then the same property of all
 * associated view providers of the objects of the object group get changed as well.
 */
void ViewProviderPart::onChanged(const App::Property* prop) {
    ViewProviderDragger::onChanged(prop);
}

void ViewProviderPart::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Toggle active part"));
    func->trigger(act, [this](){
        this->doubleClicked();
    });

    ViewProviderDragger::setupContextMenu(menu, receiver, member);
}

bool ViewProviderPart::doubleClicked()
{
    //make the part the active one

    //first, check if the part is already active.
    App::DocumentObject* activePart = nullptr;
    auto activeDoc = Gui::Application::Instance->activeDocument();
    if(!activeDoc)
        activeDoc = getDocument();
    auto activeView = activeDoc->setActiveView(this);
    if(!activeView)
        return false;

    activePart = activeView->getActiveObject<App::DocumentObject*> (PARTKEY);

    if (activePart == this->getObject()){
        //active part double-clicked. Deactivate.
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s', None)",
                PARTKEY);
    } else {
        //set new active part
        Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.ActiveDocument.ActiveView.setActiveObject('%s', App.getDocument('%s').getObject('%s'))",
                PARTKEY,
                this->getObject()->getDocument()->getName(),
                this->getObject()->getNameInDocument());
    }

    return true;
}

QIcon ViewProviderPart::getIcon() const
{
    // the original Part object for this ViewProviderPart
    auto part = static_cast<App::Part*>(this->getObject());
    // the normal case for Std_Part
    const char* pixmap = sPixmap;
    // if it's flagged as an Assembly in its Type, it gets another icon
    if (part->Type.getStrValue() == "Assembly") { pixmap = aPixmap; }

    return mergeGreyableOverlayIcons (Gui::BitmapFactory().pixmap(pixmap));
}


// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPartPython, Gui::ViewProviderPart)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderPart>;
}
