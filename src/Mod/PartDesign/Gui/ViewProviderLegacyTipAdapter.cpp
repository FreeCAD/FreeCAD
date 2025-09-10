/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
#include "ViewProviderLegacyTipAdapter.h"

#include <QString>
#include <QTimer>
#include <cstring>

#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <App/PropertyStandard.h>  // App::PropertyBool
#include <App/PropertyLinks.h>
#include <App/Document.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderLegacyTipAdapter,
                PartDesignGui::ViewProvider)

ViewProviderLegacyTipAdapter::ViewProviderLegacyTipAdapter() = default;

QIcon ViewProviderLegacyTipAdapter::getIcon() const
{
    // Mirror icon from BaseFeature if available
    if (auto* obj = this->getObject()) {
        if (auto* link = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("BaseFeature"))) {
            if (auto* base = link->getValue()) {
                if (auto* vp = Gui::Application::Instance
                               ? Gui::Application::Instance->getViewProvider(base) : nullptr) {
                    if (auto* vpd = dynamic_cast<Gui::ViewProvider*>(vp))
                        return vpd->getIcon();
                }
            }
        }
    }
    // Fallback
    QIcon ic = Gui::BitmapFactory().iconFromTheme("PartDesign_Feature");
    if (!ic.isNull()) return ic;
    return QIcon(QStringLiteral(":/icons/PartDesign_SubShapeBinder.svg"));
}


std::vector<App::DocumentObject*>
ViewProviderLegacyTipAdapter::claimChildren() const
{
    std::vector<App::DocumentObject*> kids;
    if (auto* obj = this->getObject()) {
        if (auto* link = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("BaseFeature"))) {
            if (auto* base = link->getValue())
                kids.push_back(base);
        }
    }
    return kids;
}

void ViewProviderLegacyTipAdapter::attach(App::DocumentObject* obj)
{
    PartDesignGui::ViewProvider::attach(obj);

    // Hide BaseFeature immediately (tree+3D) if it already exists
    if (!obj || !Gui::Application::Instance) return;
    if (auto* link = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("BaseFeature"))) {
        if (auto* base = link->getValue()) {
            if (auto* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                    Gui::Application::Instance->getViewProvider(base))) {
                vp->hide();
                if (auto* vis = dynamic_cast<App::PropertyBool*>(vp->getPropertyByName("Visibility")))
                    vis->setValue(false);
            }
        }
    }
}


void ViewProviderLegacyTipAdapter::onChanged(const App::Property* prop)
{
    // Keep default PD behavior: when this object becomes visible, PD hides other PD::Feature siblings
    PartDesignGui::ViewProvider::onChanged(prop);
    if (!prop) return;

    // If our own Visibility flips ON or BaseFeature changes, ensure the BaseFeature VP is hidden
    const bool becameVisible = (prop == &Visibility && Visibility.getValue());
    const bool baseChanged   = (std::strcmp(prop->getName(), "BaseFeature") == 0);
    if (!becameVisible && !baseChanged) return;

    if (!Gui::Application::Instance) return;
    if (auto* obj = this->getObject()) {
        if (auto* link = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("BaseFeature"))) {
            if (auto* base = link->getValue()) {
                if (auto* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                        Gui::Application::Instance->getViewProvider(base))) {
                    vp->hide();
                    if (auto* vis = dynamic_cast<App::PropertyBool*>(vp->getPropertyByName("Visibility")))
                        vis->setValue(false);
                }
            }
        }
    }
}


