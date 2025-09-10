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
    auto getIconOf = [](App::DocumentObject* obj) -> QIcon {
        if (!obj || !Gui::Application::Instance) return {};
        if (auto* vp = Gui::Application::Instance->getViewProvider(obj))
            if (auto* vpd = dynamic_cast<Gui::ViewProvider*>(vp))
                return vpd->getIcon();
        return {};
    };

    if (auto* obj = getObject()) {
        // Prefer BaseFeature (if set), else BaseObject
        if (auto* pf = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("BaseFeature"))) {
            if (auto ic = getIconOf(pf->getValue()); !ic.isNull()) return ic;
        }
        if (auto* po = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("BaseObject"))) {
            if (auto ic = getIconOf(po->getValue()); !ic.isNull()) return ic;
        }
    }

    auto ic = Gui::BitmapFactory().iconFromTheme("PartDesign_Feature");
    return ic.isNull() ? QIcon(QStringLiteral(":/icons/PartDesign_Feature.svg")) : ic;
}

std::vector<App::DocumentObject*> ViewProviderLegacyTipAdapter::claimChildren() const
{
    std::vector<App::DocumentObject*> kids;
    if (auto* o = getObject()) {
        if (auto* pf = dynamic_cast<App::PropertyLink*>(o->getPropertyByName("BaseFeature")))
            if (auto* v = pf->getValue()) kids.push_back(v);
        if (auto* po = dynamic_cast<App::PropertyLink*>(o->getPropertyByName("BaseObject")))
            if (auto* v = po->getValue()) kids.push_back(v);
    }
    return kids;
}


void ViewProviderLegacyTipAdapter::attach(App::DocumentObject* o)
{
    PartDesignGui::ViewProvider::attach(o);

    auto hideChild = [](App::DocumentObject* ch) {
        if (!ch || !Gui::Application::Instance) return;
        if (auto* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                Gui::Application::Instance->getViewProvider(ch))) {
            vp->hide();
            if (auto* vis = dynamic_cast<App::PropertyBool*>(vp->getPropertyByName("Visibility")))
                vis->setValue(false);
        }
    };

    if (!o) return;
    if (auto* pf = dynamic_cast<App::PropertyLink*>(o->getPropertyByName("BaseFeature")))
        hideChild(pf->getValue());
    if (auto* po = dynamic_cast<App::PropertyLink*>(o->getPropertyByName("BaseObject")))
        hideChild(po->getValue());
}

void ViewProviderLegacyTipAdapter::onChanged(const App::Property* prop)
{
    PartDesignGui::ViewProvider::onChanged(prop);
    if (!prop) return;

    const bool becameVisible = (prop == &Visibility && Visibility.getValue());
    const bool baseChanged   = std::strcmp(prop->getName(), "BaseFeature") == 0
                            || std::strcmp(prop->getName(), "BaseObject")  == 0;

    if (!becameVisible && !baseChanged) return;

    attach(getObject()); // reuse the same hide logic
}

