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
#include <App/OriginFeature.h>
#include <App/Document.h>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "ViewProviderPart.h"
#include "ViewProviderOrigin.h"
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


PROPERTY_SOURCE(Gui::ViewProviderPart, Gui::ViewProviderGeoFeatureGroup)


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
    Gui::Document* gdoc = Gui::Application::Instance->getDocument ( getObject()->getDocument() );
    App::Part* part = static_cast<App::Part*>(pcObject);
    if ( &obj == pcObject || (
        obj.getTypeId() != App::Origin::getClassTypeId() &&
        obj.getTypeId() != App::Plane::getClassTypeId() &&
        obj.getTypeId() != App::Line::getClassTypeId() ) ) {

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

        //get the bounding box values
        SbVec3f max = bbox.getMax();
        SbVec3f min = bbox.getMin();

        auto origins = part->getObjectsOfType(App::Origin::getClassTypeId());
        if (origins.empty())
          return;
        App::Origin* origin = dynamic_cast<App::Origin*>(origins.front());
        if(!origin)
            return;


        Base::Vector3d size;
        for (uint_fast8_t i=0; i<3; i++) {
            size[i] = std::max ( fabs ( max[i] ), fabs ( min[i] ) );
            if (size[i] < 1e-7) { // TODO replace it with some non-magick value (2015-08-31, Fat-Zer)
                size[i] = ViewProviderOrigin::defaultSize();
            }
        }

        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(origin);
        if (vp) {
            assert ( vp->isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) );
            Gui::ViewProviderOrigin *vpOrigin = static_cast<Gui::ViewProviderOrigin *> (vp);
            vpOrigin->Size.setValue ( size * 1.3 );
        }
    }
}


bool ViewProviderPart::doubleClicked(void)
{
    if(Workbench.getStrValue() != "") {
        // assure the given workbench
        Gui::Command::assureWorkbench( Workbench.getValue() );
    }

    //make the part the active one
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PARTKEY, this->getObject()->getNameInDocument());

    return true;
}

// commented out for thurther rewrite (2015-09-01, Fat-Zer)
// bool ViewProviderPart::onDelete(const std::vector<std::string> &)
// {
//     if(getActiveView()->getActiveObject<App::Part*>(PARTKEY) == getObject())
//         Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', None)", PARTKEY);
//
//     return true;
// }

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
	// add the origin at the root of the Part
    // first check if they already exist
    std::vector<App::DocumentObject*> origins = part->getObjectsOfType(App::Origin::getClassTypeId());

    if ( origins.empty() ) {
        std::string oname = part->getDocument()->getUniqueObjectName("Origin");
        Gui::Command::doCommand(Gui::Command::Doc,"Origin = App.activeDocument().addObject('App::Origin','%s')", oname.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"Origin.Label = '%s'", QObject::tr("Origin").toStdString().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().%s.addObject(Origin)", part->getNameInDocument());
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
