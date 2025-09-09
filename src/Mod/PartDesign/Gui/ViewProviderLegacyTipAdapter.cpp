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
    const QIcon ic = Gui::BitmapFactory().iconFromTheme("PartDesign_SubShapeBinder");
    if (!ic.isNull())
        return ic;
    return QIcon(QStringLiteral(":/icons/PartDesign_SubShapeBinder.svg"));
}

std::vector<App::DocumentObject*>
ViewProviderLegacyTipAdapter::claimChildren() const
{
    std::vector<App::DocumentObject*> kids;
    if (auto* obj = this->getObject()) {
        if (auto* pl = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("Binder"))) {
            if (auto* ch = pl->getValue())
                kids.push_back(ch);
        }
    }
    return kids;
}

void ViewProviderLegacyTipAdapter::attach(App::DocumentObject* obj)
{
    PartDesignGui::ViewProvider::attach(obj);

    if (!obj) return;

    // Try now; if child's VP isn't there yet, queue retries.
    if (!hideChildBinderVP(obj))
        hideChildBinderVPLater(obj);
}

void ViewProviderLegacyTipAdapter::onChanged(const App::Property* prop)
{
    // Keep base behavior (hides other PD::Feature siblings when this turns visible)
    PartDesignGui::ViewProvider::onChanged(prop);

    if (!prop) return;

    // 1) If our own Visibility flips ON, immediately hide the child binder
    if (prop == &Visibility && Visibility.getValue()) {
        if (!hideChildBinderVP(this->getObject()))
            hideChildBinderVPLater(this->getObject());
        return;
    }

    // 2) If the Binder link changes, hide the (new) child too
    if (std::strcmp(prop->getName(), "Binder") == 0) {
        if (!hideChildBinderVP(this->getObject()))
            hideChildBinderVPLater(this->getObject());
        return;
    }
}

bool ViewProviderLegacyTipAdapter::onDelete(const std::vector<std::string>& what)
{
    if (auto* obj = this->getObject()) {
        if (auto* pl = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("Binder"))) {
            if (auto* ch = pl->getValue()) {
                if (auto* doc = obj->getDocument())
                    doc->removeObject(ch->getNameInDocument());
            }
        }
    }
    return PartDesignGui::ViewProvider::onDelete(what);
}

bool ViewProviderLegacyTipAdapter::hideChildBinderVP(App::DocumentObject* obj) const
{
    if (!obj || !Gui::Application::Instance)
        return false;

    auto* pl = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("Binder"));
    if (!pl) return false;

    App::DocumentObject* ch = pl->getValue();
    if (!ch) return false;

    // Fetch the child's VP directly from the application (independent of GUI doc timing)
    if (auto* vpChild = dynamic_cast<Gui::ViewProviderDocumentObject*>(
            Gui::Application::Instance->getViewProvider(ch))) {

        // Hide in 3D
        vpChild->hide();
        // Close the eye in the tree
        if (auto* vis = dynamic_cast<App::PropertyBool*>(vpChild->getPropertyByName("Visibility")))
            vis->setValue(false);
        // Optional: keep it from being picked
        if (auto* sel = dynamic_cast<App::PropertyBool*>(vpChild->getPropertyByName("Selectable")))
            sel->setValue(false);

        return true;
    }
    return false;
}

void ViewProviderLegacyTipAdapter::hideChildBinderVPLater(App::DocumentObject* obj) const
{
    // Re-try shortly; binder VP is sometimes created after we attach
    auto retry = [this, obj]() { this->hideChildBinderVP(obj); };
    QTimer::singleShot(0, retry);
    QTimer::singleShot(50, retry);
    QTimer::singleShot(150, retry);
}

