/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <QMessageBox>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GroupExtension.h>
#include <Base/Console.h>
#include <Base/Tools.h>

#include "ViewProviderGroupExtension.h"
#include "ViewProviderDocumentObject.h"
#include "Command.h"
#include "Document.h"
#include "MainWindow.h"


using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderGroupExtension, Gui::ViewProviderExtension)

ViewProviderGroupExtension::ViewProviderGroupExtension()
{
    initExtensionType(ViewProviderGroupExtension::getExtensionClassTypeId());
}

ViewProviderGroupExtension::~ViewProviderGroupExtension() = default;

bool ViewProviderGroupExtension::extensionCanDragObjects() const {
    return true;
}

bool ViewProviderGroupExtension::extensionCanDragObject(App::DocumentObject*) const {

    //we can drag anything out
    return true;
}

void ViewProviderGroupExtension::extensionDragObject(App::DocumentObject* obj) {

    Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").removeObject("
            "App.getDocument(\"%s\").getObject(\"%s\"))",
            getExtendedViewProvider()->getObject()->getDocument()->getName(), getExtendedViewProvider()->getObject()->getNameInDocument(),
            obj->getDocument()->getName(), obj->getNameInDocument() );
}

bool ViewProviderGroupExtension::extensionCanDropObjects() const {
    return true;
}

bool ViewProviderGroupExtension::extensionCanDropObject(App::DocumentObject* obj) const {

#ifdef FC_DEBUG
    Base::Console().Log("Check ViewProviderGroupExtension");
#endif

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();

    //we cannot drop thing of this group into it again
    if (group->hasObject(obj))
        return false;

    if (group->allowObject(obj))
        return true;

    return false;
}

void ViewProviderGroupExtension::extensionDropObject(App::DocumentObject* obj) {

    auto grp = static_cast<App::DocumentObject*>(getExtendedViewProvider()->getObject());
    App::Document* doc = grp->getDocument();

    // build Python command for execution
    QString cmd;
    cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").addObject("
                        "App.getDocument(\"%1\").getObject(\"%3\"))")
                        .arg(QString::fromLatin1(doc->getName()),
                             QString::fromLatin1(grp->getNameInDocument()),
                             QString::fromLatin1(obj->getNameInDocument()));

    Gui::Command::doCommand(Gui::Command::App, cmd.toUtf8());
}

std::vector< App::DocumentObject* > ViewProviderGroupExtension::extensionClaimChildren() const {

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
    return group->Group.getValues();
}

void ViewProviderGroupExtension::extensionShow() {

    // avoid possible infinite recursion
    if (guard)
        return;
    Base::StateLocker lock(guard);

    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    if (!getExtendedViewProvider()->isRestoring() ) {
        auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
        for(auto obj : group->Group.getValues()) {
            if(obj && !obj->Visibility.getValue())
                obj->Visibility.setValue(true);
        }
    }

    ViewProviderExtension::extensionShow();
}

void ViewProviderGroupExtension::extensionHide() {

    // avoid possible infinite recursion
    if (guard)
        return;
    Base::StateLocker lock(guard);

    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    //
    // Property::User1 is used by ViewProviderDocumentObject to mark for
    // temporary visibility changes. Do not propagate the change to children.
    if (!getExtendedViewProvider()->isRestoring()
            && !getExtendedViewProvider()->Visibility.testStatus(App::Property::User1))
    {
        auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
        for(auto obj : group->Group.getValues()) {
            if(obj && obj->Visibility.getValue())
                obj->Visibility.setValue(false);
        }
    }
    ViewProviderExtension::extensionHide();
}

bool ViewProviderGroupExtension::extensionOnDelete(const std::vector< std::string >& ) {

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
    // If the group is nonempty ask the user if they want to delete its content
    if (group->Group.getSize() > 0) {
        QMessageBox::StandardButton choice =
            QMessageBox::question(getMainWindow(), QObject::tr ( "Delete group content?" ),
                QObject::tr ( "The %1 is not empty, delete its content as well?")
                    .arg ( QString::fromUtf8 ( getExtendedViewProvider()->getObject()->Label.getValue () ) ),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );

        if (choice == QMessageBox::Yes) {
            Gui::Command::doCommand(Gui::Command::Doc,
                    "App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
                    , getExtendedViewProvider()->getObject()->getDocument()->getName()
                    , getExtendedViewProvider()->getObject()->getNameInDocument());
        }
    }
    return true;
}


namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGroupExtensionPython, Gui::ViewProviderGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderGroupExtension>;
}
