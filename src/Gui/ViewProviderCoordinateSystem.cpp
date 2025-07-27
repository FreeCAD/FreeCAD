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
    auto* lcs = getObject<App::LocalCoordinateSystem>();
    if (!lcs) {
        return {};
    }

    std::vector<App::DocumentObject*> childs = lcs->OriginFeatures.getValues();
    auto it = std::find(childs.begin(), childs.end(), lcs);
    if (it != childs.end()) {
        childs.erase(it);
    }
    return childs;
}

std::vector<App::DocumentObject*> ViewProviderCoordinateSystem::claimChildren3D() const
{
    return claimChildren();
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

void ViewProviderCoordinateSystem::setTemporaryVisibility(DatumElements elements)
{
    auto* lcs = getObject<App::LocalCoordinateSystem>();
    if (!lcs) {
        return;
    }

    bool saveState = tempVisMap.empty();

    try {
        // Remember & Set axis visibility
        for(App::DocumentObject* obj : lcs->axes()) {
            if (auto vp = Gui::Application::Instance->getViewProvider(obj)) {
                if (saveState) {
                    tempVisMap[vp] = vp->isVisible();
                }
                vp->setVisible(elements.testFlag(DatumElement::Axes));
            }
        }

        // Remember & Set plane visibility
        for(App::DocumentObject* obj : lcs->planes()) {
            if (auto vp = Gui::Application::Instance->getViewProvider(obj)) {
                if (saveState) {
                    tempVisMap[vp] = vp->isVisible();
                }
                vp->setVisible(elements.testFlag(DatumElement::Planes));
            }
        }

        // Remember & Set origin point visibility
        App::DocumentObject* obj = lcs->getOrigin();
        if (auto vp = Gui::Application::Instance->getViewProvider(obj)) {
            if (saveState) {
                tempVisMap[vp] = vp->isVisible();
            }
            vp->setVisible(elements.testFlag(DatumElement::Origin));
        }
    }
    catch (const Base::Exception &ex) {
        Base::Console().error ("%s\n", ex.what() );
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

bool ViewProviderCoordinateSystem::isTemporaryVisibility()
{
    return !tempVisMap.empty();
}

void ViewProviderCoordinateSystem::setPlaneLabelVisibility(bool val)
{
    auto* lcs = getObject<App::LocalCoordinateSystem>();
    if (!lcs) {
        return;
    }
    for (auto* plane : lcs->planes()) {
        auto* vp = dynamic_cast<Gui::ViewProviderPlane*>(
            Gui::Application::Instance->getViewProvider(plane));
        if (vp) {
            vp->setLabelVisibility(val);
        }
    }
}

void ViewProviderCoordinateSystem::applyDatumObjects(const DatumObjectFunc& func)
{
    auto* lcs = getObject<App::LocalCoordinateSystem>();
    if (!lcs) {
        return;
    }
    const auto& objs = lcs->OriginFeatures.getValues();
    for (auto* obj : objs) {
        auto* vp = dynamic_cast<Gui::ViewProviderDatum*>(
            Gui::Application::Instance->getViewProvider(obj));
        if (vp) {
            func(vp);
        }
    }
}

void ViewProviderCoordinateSystem::setTemporaryScale(double factor)
{
    applyDatumObjects([factor](ViewProviderDatum* vp) {
        vp->setTemporaryScale(factor);
    });
}

void ViewProviderCoordinateSystem::resetTemporarySize()
{
    applyDatumObjects([](ViewProviderDatum* vp) {
        vp->resetTemporarySize();
    });
}

bool ViewProviderCoordinateSystem::onDelete(const std::vector<std::string> &)
{
    auto* lcs = getObject<App::LocalCoordinateSystem>();
    if (!lcs) {
        return false;
    }

    if (lcs->is<App::Origin>() && !lcs->getInList().empty()) {
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
