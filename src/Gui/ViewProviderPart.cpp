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
#include <App/Plane.h>
#include <App/Document.h>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "ViewProviderPart.h"
#include "Application.h"
#include "Command.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "Tree.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderPart, Gui::ViewProviderGeometryObject)


/**
 * Creates the view provider for an object group.
 */
ViewProviderPart::ViewProviderPart() 
{
    ADD_PROPERTY(Workbench,("PartDesignWorkbench"));
}

ViewProviderPart::~ViewProviderPart()
{
}

/**
 * Whenever a property of the group gets changed then the same property of all
 * associated view providers of the objects of the object group get changed as well.
 */
void ViewProviderPart::onChanged(const App::Property* prop)
{
    ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderPart::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);
}

void ViewProviderPart::updateData(const App::Property* prop)
{

}


bool ViewProviderPart::doubleClicked(void)
{
    if(Workbench.getValue() != "")
        // assure the PartDesign workbench
        Gui::Command::assureWorkbench( Workbench.getValue() );

    return true;
}


std::vector<std::string> ViewProviderPart::getDisplayModes(void) const
{
    // empty
    return std::vector<std::string>();
}

bool ViewProviderPart::onDelete(const std::vector<std::string> &)
{
    //Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
    //                                 ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    return true;
}


void ViewProviderPart::hide(void)
{

}

void ViewProviderPart::show(void)
{

}

bool ViewProviderPart::isShow(void) const
{
    return Visibility.getValue();
}

void ViewProviderPart::Restore(Base::XMLReader &reader)
{
    Visibility.StatusBits.set(9); // tmp. set
    ViewProviderDocumentObject::Restore(reader);
    Visibility.StatusBits.reset(9); // unset
}


/**
 * Returns the pixmap for the list item.
 */
QIcon ViewProviderPart::getIcon() const
{
    QIcon groupIcon;
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon),
                        QIcon::Normal, QIcon::Off);
    groupIcon.addPixmap(QApplication::style()->standardPixmap(QStyle::SP_DirOpenIcon),
                        QIcon::Normal, QIcon::On);
    return groupIcon;
}

void ViewProviderPart::setUpPart(const App::Part *part)
{
	// add the standard planes at the root of the Part
    // first check if they already exist
    // FIXME: If the user renames them, they won't be found...
    bool found = false;
    std::vector<App::DocumentObject*> planes = part->getObjectsOfType(App::Plane::getClassTypeId());
    for (std::vector<App::DocumentObject*>::const_iterator p = planes.begin(); p != planes.end(); p++) {
        for (unsigned i = 0; i < 3; i++) {
            if (strcmp(App::Part::BaseplaneTypes[i], dynamic_cast<App::Plane*>(*p)->PlaneType.getValue()) == 0) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        // ... and put them in the 'Origin' group
        //Gui::Command::doCommand(Gui::Command::Doc,"OGroup = App.activeDocument().addObject('App::DocumentObjectGroup','%s')", "Origin");
        //Gui::Command::doCommand(Gui::Command::Doc,"OGroup.Label = '%s'", QObject::tr("Origin").toStdString().c_str());
        // Add the planes ...
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", App::Part::BaseplaneTypes[0]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("XY-Plane").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", part->getNameInDocument());
        //Gui::Command::doCommand(Gui::Command::Doc,"OGroup.addObject(App.activeDocument().ActiveObject)");
        
		Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", App::Part::BaseplaneTypes[1]);
		Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(1,0,0),-90))");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("XZ-Plane").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", part->getNameInDocument());
        //Gui::Command::doCommand(Gui::Command::Doc,"OGroup.addObject(App.activeDocument().ActiveObject)");

		Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", App::Part::BaseplaneTypes[2]);
		Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(0,1,0),90))");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("YZ-Plane").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", part->getNameInDocument());
        //Gui::Command::doCommand(Gui::Command::Doc,"OGroup.addObject(App.activeDocument().ActiveObject)");
        
		//Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(OGroup)", part->getNameInDocument());
        // TODO: Fold the group (is that possible through the Python interface?)
    }

}


// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPartPython, Gui::ViewProviderPart)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderPart>;
}
