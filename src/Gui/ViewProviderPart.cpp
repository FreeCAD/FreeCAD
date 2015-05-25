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
#include <App/Origin.h>
#include <App/Plane.h>
#include <App/Line.h>
#include <App/Document.h>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "ViewProviderPart.h"
#include "ViewProviderPlane.h"
#include "ViewProviderLine.h"
#include "Application.h"
#include "Command.h"
#include "BitmapFactory.h"
#include "Document.h"
#include "Tree.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

#include "Base/Console.h"
#include <boost/bind.hpp>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoSeparator.h>

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
    connection.disconnect();
}

/**
 * Whenever a property of the group gets changed then the same property of all
 * associated view providers of the objects of the object group get changed as well.
 */
void ViewProviderPart::onChanged(const App::Property* prop)
{
    ViewProviderGeoFeatureGroup::onChanged(prop);
}

void ViewProviderPart::attach(App::DocumentObject *pcObj)
{
    connection = pcObj->getDocument()->signalChangedObject.connect(boost::bind(&ViewProviderPart::onObjectChanged, this, _1, _2));
    ViewProviderGeoFeatureGroup::attach(pcObj);
}

void ViewProviderPart::updateData(const App::Property* prop)
{
    ViewProviderGeoFeatureGroup::updateData(prop);
}

void ViewProviderPart::onObjectChanged(const App::DocumentObject& obj, const App::Property&)
{
    App::Part* part = static_cast<App::Part*>(pcObject);
    if(static_cast<App::Part*>(pcObject)->hasObject(&obj) && 
        obj.getTypeId() != App::Origin::getClassTypeId() &&
        obj.getTypeId() != App::Plane::getClassTypeId() &&
        obj.getTypeId() != App::Line::getClassTypeId()) {
        
        View3DInventorViewer* viewer = static_cast<View3DInventor*>(this->getActiveView())->getViewer();
        SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());
        
        //calculate for everything but datums
        SbBox3f bbox(1e-9, 1e-9, 1e-9, 1e-9, 1e-9, 1e-9);
        for(App::DocumentObject* obj : part->getObjects()) {
            if(obj->getTypeId() != App::Origin::getClassTypeId() &&
               obj->getTypeId() != App::Plane::getClassTypeId() && 
               obj->getTypeId() != App::Line::getClassTypeId() ) {

                //getting crash on deletion PartDesign::Body object. no viewprovider.
                ViewProvider *viewProvider = Gui::Application::Instance->getViewProvider(obj);
                if (!viewProvider)
                  continue;
                
                bboxAction.apply(viewProvider->getRoot());
                bbox.extendBy(bboxAction.getBoundingBox());
            }
        };
        
        if(bbox.getSize().length() < 1e-7) {
            bbox = SbBox3f(10., 10., 10., 10., 10., 10.);
        }
        
        //get the bounding box values
        SbVec3f size = bbox.getSize()*1.3;
        SbVec3f max = bbox.getMax()*1.3;
        SbVec3f min = bbox.getMin()*1.3;
       
        auto origins = part->getObjectsOfType(App::Origin::getClassTypeId());
        if (origins.empty())
          return;
        App::Origin* origin = dynamic_cast<App::Origin*>(origins.front());
        if(!origin)
            return;
        
        //get the planes and set their values
        std::vector<App::DocumentObject*> planes = origin->getObjectsOfType(App::Plane::getClassTypeId());
        for (std::vector<App::DocumentObject*>::const_iterator p = planes.begin(); p != planes.end(); p++) {
 
             Gui::ViewProviderPlane* vp = dynamic_cast<Gui::ViewProviderPlane*>(Gui::Application::Instance->getViewProvider(*p));
             if(vp) {
                if (strcmp(App::Part::BaseplaneTypes[0], dynamic_cast<App::Plane*>(*p)->PlaneType.getValue()) == 0)
                        vp->Size.setValue(std::max(std::abs(std::min(min[0], min[1])),std::max(max[0], max[1])));
                else if (strcmp(App::Part::BaseplaneTypes[1], dynamic_cast<App::Plane*>(*p)->PlaneType.getValue()) == 0)
                        vp->Size.setValue(std::max(std::abs(std::min(min[0], min[2])),std::max(max[0], max[2])));
                else if (strcmp(App::Part::BaseplaneTypes[2], dynamic_cast<App::Plane*>(*p)->PlaneType.getValue()) == 0)               
                        vp->Size.setValue(std::max(std::abs(std::min(min[1], min[2])),std::max(max[1], max[2]))); 
             }
        }
        
        //get the lines and set their values
        std::vector<App::DocumentObject*> lines = origin->getObjectsOfType(App::Line::getClassTypeId());
        for (std::vector<App::DocumentObject*>::const_iterator p = lines.begin(); p != lines.end(); p++) {
 
             Gui::ViewProviderLine* vp = dynamic_cast<Gui::ViewProviderLine*>(Gui::Application::Instance->getViewProvider(*p));
             if(vp) {
                if (strcmp(App::Part::BaselineTypes[0], dynamic_cast<App::Line*>(*p)->LineType.getValue()) == 0)
                        vp->Size.setValue(std::max(std::abs(std::min(min[0], min[1])),std::max(max[0], max[1])));
                else if (strcmp(App::Part::BaselineTypes[1], dynamic_cast<App::Line*>(*p)->LineType.getValue()) == 0) 
                        vp->Size.setValue(std::max(std::abs(std::min(min[0], min[2])),std::max(max[0], max[2])));
                else if (strcmp(App::Part::BaselineTypes[2], dynamic_cast<App::Line*>(*p)->LineType.getValue()) == 0)
                        vp->Size.setValue(std::max(std::abs(std::min(min[1], min[2])),std::max(max[1], max[2])));
             }
        }
    }
}


bool ViewProviderPart::doubleClicked(void)
{
    if(Workbench.getValue() != "")
        // assure the PartDesign workbench
        Gui::Command::assureWorkbench( Workbench.getValue() );

    return true;
}


bool ViewProviderPart::onDelete(const std::vector<std::string> &)
{
    //Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
    //                                 ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    return true;
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
        std::string oname = part->getDocument()->getUniqueObjectName("Origin");
        Gui::Command::doCommand(Gui::Command::Doc,"Origin = App.activeDocument().addObject('App::Origin','%s')", oname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"Origin.Label = '%s'", QObject::tr("Origin").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(Origin)", part->getNameInDocument());
        
        // Add the planes ...
        std::string pname = part->getDocument()->getUniqueObjectName(App::Part::BaseplaneTypes[0]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", pname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.PlaneType = '%s'", App::Part::BaseplaneTypes[0]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("XY-Plane").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", oname.c_str());
        
        pname = part->getDocument()->getUniqueObjectName(App::Part::BaseplaneTypes[1]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", pname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(1,0,0),90))");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.PlaneType = '%s'", App::Part::BaseplaneTypes[1]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("XZ-Plane").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", oname.c_str());
        
        pname = part->getDocument()->getUniqueObjectName(App::Part::BaseplaneTypes[2]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Plane','%s')", pname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(1,1,1),120))");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.PlaneType = '%s'", App::Part::BaseplaneTypes[2]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("YZ-Plane").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", oname.c_str());
        
        // Add the lines ...
        std::string lname = part->getDocument()->getUniqueObjectName(App::Part::BaselineTypes[0]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Line','%s')", lname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.LineType = '%s'", App::Part::BaselineTypes[0]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("X-Axis").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", oname.c_str());
        
        lname = part->getDocument()->getUniqueObjectName(App::Part::BaselineTypes[1]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Line','%s')", lname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(0,0,1),90))");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.LineType = '%s'", App::Part::BaselineTypes[1]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("Y-Axis").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", oname.c_str());
        
        lname = part->getDocument()->getUniqueObjectName(App::Part::BaselineTypes[2]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject('App::Line','%s')", lname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Placement = App.Placement(App.Vector(),App.Rotation(App.Vector(0,1,0),90))");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.LineType = '%s'", App::Part::BaselineTypes[2]);
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().ActiveObject.Label = '%s'", QObject::tr("Z-Axis").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().ActiveObject)", oname.c_str());
   
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
