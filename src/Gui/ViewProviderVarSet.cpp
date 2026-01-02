/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/


#include <memory>

#include <QTimer>

#include <App/Application.h>
#include <App/VarSet.h>

#include "MainWindow.h"
#include "ViewProviderVarSet.h"

using namespace Gui;
using namespace Gui::Dialog;

PROPERTY_SOURCE(Gui::ViewProviderVarSet, Gui::ViewProviderDocumentObject)

ViewProviderVarSet::ViewProviderVarSet()
{
    setToggleVisibility(ToggleVisibilityMode::NoToggleVisibility);
    sPixmap = "VarSet";
}

std::string ViewProviderVarSet::getTreeLabel() const
{
    std::string baseLabel = ViewProviderDocumentObject::getTreeLabel();
    
    auto* obj = getObject();
    if (!obj || !obj->isAttachedToDocument()) {
        return baseLabel;
    }
    
    // Count dynamic properties (exclude built-in properties)
    auto dynamicNames = obj->getDynamicPropertyNames();
    int count = static_cast<int>(dynamicNames.size());
    
    // Append count if there are dynamic properties
    if (count > 0) {
        return baseLabel + " (" + std::to_string(count) + ")";
    }
    
    return baseLabel;
}

void ViewProviderVarSet::attach(App::DocumentObject* pcObject)
{
    ViewProviderDocumentObject::attach(pcObject);
    
    // Connect to application-level signals for dynamic property changes
    connPropertyAdded = App::GetApplication().signalAppendDynamicProperty.connect(
        [this](const App::Property& prop) { onDynamicPropertyAdded(prop); }
    );
    connPropertyRemoved = App::GetApplication().signalRemoveDynamicProperty.connect(
        [this](const App::Property& prop) { onDynamicPropertyRemoved(prop); }
    );
}

void ViewProviderVarSet::onDynamicPropertyAdded(const App::Property& prop)
{
    // Only update if the property belongs to our object
    if (prop.getContainer() == getObject()) {
        signalChangeTreeLabel();
    }
}

void ViewProviderVarSet::onDynamicPropertyRemoved(const App::Property& prop)
{
    // Only update if the property belongs to our object
    // Use deferred update because this signal fires BEFORE the property is removed
    if (prop.getContainer() == getObject()) {
        QTimer::singleShot(0, [this]() {
            signalChangeTreeLabel();
        });
    }
}

bool ViewProviderVarSet::doubleClicked()
{
    if (!dialog) {
        dialog = std::make_unique<DlgAddProperty>(getMainWindow(), this);
    }

    // Do not use exec() here because it blocks and prevents command Std_VarSet
    // to commit the autotransaction.  This in turn prevents the dialog to
    // handle transactions well.
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();

    return true;
}

void ViewProviderVarSet::onFinished(int /*result*/)
{
    dialog = nullptr;
}
