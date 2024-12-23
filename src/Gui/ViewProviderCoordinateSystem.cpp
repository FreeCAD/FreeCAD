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
# include <QMessageBox>
# include <QCheckBox>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include "Base/Console.h"
#include <Base/Vector3D.h>

#include "ViewProviderCoordinateSystem.h"
#include "Application.h"
#include "Command.h"
#include "Document.h"
#include "ViewProviderLine.h"
#include "ViewProviderPlane.h"
#include "ViewProviderPoint.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderCoordinateSystem, Gui::ViewProviderGeoFeatureGroup)

/**
 * Creates the view provider for an object group.
 */
ViewProviderCoordinateSystem::ViewProviderCoordinateSystem()
{
    sPixmap = "Std_CoordinateSystem";
    Visibility.setValue(false);

    pcGroupChildren = new SoGroup();
    pcGroupChildren->ref();

    auto lm = new SoLightModel();
    lm->model = SoLightModel::BASE_COLOR;
    pcRoot->insertChild(lm, 0);
}

ViewProviderCoordinateSystem::~ViewProviderCoordinateSystem() {
    pcGroupChildren->unref();
    pcGroupChildren = nullptr;
}

std::vector<App::DocumentObject*> ViewProviderCoordinateSystem::claimChildren() const
{
    return static_cast<App::Origin*>( getObject() )->OriginFeatures.getValues();
}

std::vector<App::DocumentObject*> ViewProviderCoordinateSystem::claimChildren3D() const {
    return claimChildren ();
}

void ViewProviderCoordinateSystem::attach(App::DocumentObject* pcObject)
{
    Gui::ViewProviderDocumentObject::attach(pcObject);
    addDisplayMaskMode(pcGroupChildren, "Base");
}

std::vector<std::string> ViewProviderCoordinateSystem::getDisplayModes() const
{
    return { "Base" };
}

void ViewProviderCoordinateSystem::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderCoordinateSystem::setTemporaryVisibility(bool axis, bool plane) {
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

        // Remember & Set origin point visibility
        App::DocumentObject* obj = origin->getOrigin();
        if (obj) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(obj);
            if (vp) {
                if (saveState) {
                    tempVisMap[vp] = vp->isVisible();
                }
                vp->setVisible(plane);
            }
        }


    }
    catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what() );
    }

    // Remember & Set self visibility
    tempVisMap[this] = isVisible();
    setVisible(true);

}

void ViewProviderCoordinateSystem::resetTemporaryVisibility() {
    for(std::pair<Gui::ViewProvider*, bool> pair : tempVisMap) {
        pair.first->setVisible(pair.second);
    }
    tempVisMap.clear ();
}

double ViewProviderCoordinateSystem::defaultSize()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    return hGrp->GetFloat("DatumsSize", 25);
}

bool ViewProviderCoordinateSystem::isTemporaryVisibility() {
    return !tempVisMap.empty();
}

void ViewProviderCoordinateSystem::updateData(const App::Property* prop) {
    auto* jcs = dynamic_cast<App::LocalCoordinateSystem*>(getObject());
    if(jcs) {
        if (prop == &jcs->Placement) {
            // Update position
        }
    }
    ViewProviderDocumentObject::updateData(prop);
}

bool ViewProviderCoordinateSystem::onDelete(const std::vector<std::string> &) {
    auto lcs = static_cast<App::LocalCoordinateSystem*>(getObject());

    auto origin = dynamic_cast<App::Origin*>(lcs);
    if (origin && !origin->getInList().empty()) {
        // Do not allow deletion of origin objects that are not lost.
        return false;
    }

    auto objs = lcs->OriginFeatures.getValues();
    lcs->OriginFeatures.setValues({});

    for (auto obj: objs ) {
        Gui::Command::doCommand( Gui::Command::Doc, "App.getDocument(\"%s\").removeObject(\"%s\")",
                obj->getDocument()->getName(), obj->getNameInDocument() );
    }

    return true;
}
