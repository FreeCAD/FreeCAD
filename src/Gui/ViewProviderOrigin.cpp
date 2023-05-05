/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <Inventor/nodes/SoLightModel.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include "Base/Console.h"
#include <Base/Vector3D.h>

#include "ViewProviderOrigin.h"
#include "Application.h"
#include "Command.h"
#include "Document.h"
#include "ViewProviderLine.h"
#include "ViewProviderPlane.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderOrigin, Gui::ViewProviderDocumentObject)

/**
 * Creates the view provider for an object group.
 */
ViewProviderOrigin::ViewProviderOrigin()
{
    ADD_PROPERTY_TYPE ( Size, (Base::Vector3d(10,10,10)), 0, App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "The displayed size of the origin"));
    Size.setStatus(App::Property::ReadOnly, true);

    sPixmap = "Std_CoordinateSystem";
    Visibility.setValue(false);

    pcGroupChildren = new SoGroup();
    pcGroupChildren->ref();

    auto lm = new SoLightModel();
    lm->model = SoLightModel::BASE_COLOR;
    pcRoot->insertChild(lm, 0);
}

ViewProviderOrigin::~ViewProviderOrigin() {
    pcGroupChildren->unref();
    pcGroupChildren = nullptr;
}

std::vector<App::DocumentObject*> ViewProviderOrigin::claimChildren() const {
    return static_cast<App::Origin*>( getObject() )->OriginFeatures.getValues ();
}

std::vector<App::DocumentObject*> ViewProviderOrigin::claimChildren3D() const {
    return claimChildren ();
}

void ViewProviderOrigin::attach(App::DocumentObject* pcObject)
{
    Gui::ViewProviderDocumentObject::attach(pcObject);
    addDisplayMaskMode(pcGroupChildren, "Base");
}

std::vector<std::string> ViewProviderOrigin::getDisplayModes() const
{
    return { "Base" };
}

void ViewProviderOrigin::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderOrigin::setTemporaryVisibility(bool axis, bool plane) {
    auto origin = static_cast<App::Origin*>( getObject() );

    bool saveState = tempVisMap.empty();

    try {
        // Remember & Set axis visibility
        for(App::DocumentObject* obj : origin->axes()) {
            if (obj) {
                Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
                if(vp) {
                    if (saveState) {
                        tempVisMap[vp] = vp->isVisible();
                    }
                    vp->setVisible(axis);
                }
            }
        }

        // Remember & Set plane visibility
        for(App::DocumentObject* obj : origin->planes()) {
            if (obj) {
                Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
                if(vp) {
                    if (saveState) {
                        tempVisMap[vp] = vp->isVisible();
                    }
                    vp->setVisible(plane);
                }
            }
        }
    } catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what() );
    }

    // Remember & Set self visibility
    tempVisMap[this] = isVisible();
    setVisible(true);

}

void ViewProviderOrigin::resetTemporaryVisibility() {
    for(std::pair<Gui::ViewProvider*, bool> pair : tempVisMap) {
        pair.first->setVisible(pair.second);
    }
    tempVisMap.clear ();
}

double ViewProviderOrigin::defaultSize()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    return 0.25 * hGrp->GetFloat("NewDocumentCameraScale",100.0);
}

bool ViewProviderOrigin::isTemporaryVisibility() {
    return !tempVisMap.empty();
}

void ViewProviderOrigin::onChanged(const App::Property* prop) {
    if (prop == &Size) {
        try {
            Gui::Application *app = Gui::Application::Instance;
            Base::Vector3d sz = Size.getValue ();
            auto origin = static_cast<App::Origin*> ( getObject() );

            // Calculate axes and planes sizes
            double szXY = std::max ( sz.x, sz.y );
            double szXZ = std::max ( sz.x, sz.z );
            double szYZ = std::max ( sz.y, sz.z );

            double szX = std::min ( szXY, szXZ );
            double szY = std::min ( szXY, szYZ );
            double szZ = std::min ( szXZ, szYZ );

            // Find view providers
            Gui::ViewProviderPlane* vpPlaneXY, *vpPlaneXZ, *vpPlaneYZ;
            Gui::ViewProviderLine* vpLineX, *vpLineY, *vpLineZ;
            // Planes
            vpPlaneXY = static_cast<Gui::ViewProviderPlane *> ( app->getViewProvider ( origin->getXY () ) );
            vpPlaneXZ = static_cast<Gui::ViewProviderPlane *> ( app->getViewProvider ( origin->getXZ () ) );
            vpPlaneYZ = static_cast<Gui::ViewProviderPlane *> ( app->getViewProvider ( origin->getYZ () ) );
            // Axes
            vpLineX = static_cast<Gui::ViewProviderLine *> ( app->getViewProvider ( origin->getX () ) );
            vpLineY = static_cast<Gui::ViewProviderLine *> ( app->getViewProvider ( origin->getY () ) );
            vpLineZ = static_cast<Gui::ViewProviderLine *> ( app->getViewProvider ( origin->getZ () ) );

            // set their sizes
            if (vpPlaneXY) { vpPlaneXY->Size.setValue ( szXY ); }
            if (vpPlaneXZ) { vpPlaneXZ->Size.setValue ( szXZ ); }
            if (vpPlaneYZ) { vpPlaneYZ->Size.setValue ( szYZ ); }
            if (vpLineX) { vpLineX->Size.setValue ( szX * axesScaling ); }
            if (vpLineY) { vpLineY->Size.setValue ( szY * axesScaling ); }
            if (vpLineZ) { vpLineZ->Size.setValue ( szZ * axesScaling ); }

        } catch (const Base::Exception &ex) {
            // While restoring a document don't report errors if one of the lines or planes
            // cannot be found.
            App::Document* doc = getObject()->getDocument();
            if (!doc->testStatus(App::Document::Restoring))
                Base::Console().Error ("%s\n", ex.what() );
        }
    }

    ViewProviderDocumentObject::onChanged ( prop );
}

bool ViewProviderOrigin::onDelete(const std::vector<std::string> &) {
    auto origin = static_cast<App::Origin*>( getObject() );

    if ( !origin->getInList().empty() ) {
        return false;
    }

    auto objs = origin->OriginFeatures.getValues();
    origin->OriginFeatures.setValues({});

    for (auto obj: objs ) {
        Gui::Command::doCommand( Gui::Command::Doc, "App.getDocument(\"%s\").removeObject(\"%s\")",
                obj->getDocument()->getName(), obj->getNameInDocument() );
    }

    return true;
}
