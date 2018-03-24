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
#endif

//#include "ViewProviderGroupExtensionPy.h"
#include "ViewProviderGroupExtension.h"

#include "Command.h"
#include "Application.h"
#include "Document.h"
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GroupExtension.h>
#include <App/Expression.h>
#include <Base/Console.h>
#include <QMessageBox>

using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderGroupExtension, Gui::ViewProviderExtension)

ViewProviderGroupExtension::ViewProviderGroupExtension()  : visible(false)
{
    initExtensionType(ViewProviderGroupExtension::getExtensionClassTypeId());
}

ViewProviderGroupExtension::~ViewProviderGroupExtension()
{
}

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

    App::DocumentObject* grp = static_cast<App::DocumentObject*>(getExtendedViewProvider()->getObject());
    App::Document* doc = grp->getDocument();

    // build Python command for execution
    QString cmd;
    cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").addObject("
                        "App.getDocument(\"%1\").getObject(\"%3\"))")
                        .arg(QString::fromLatin1(doc->getName()))
                        .arg(QString::fromLatin1(grp->getNameInDocument()))
                        .arg(QString::fromLatin1(obj->getNameInDocument()));

    Gui::Command::doCommand(Gui::Command::App, cmd.toUtf8());
}

std::vector< App::DocumentObject* > ViewProviderGroupExtension::extensionClaimChildren(void) const {

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();    
    return std::vector<App::DocumentObject*>(group->Group.getValues());
}

void ViewProviderGroupExtension::extensionShow(void) {

    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    if (!getExtendedViewProvider()->isRestoring() && !this->visible) {
        auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();

        const std::vector<App::DocumentObject*> & links = group->Group.getValues();
        Gui::Document* doc = Application::Instance->getDocument(group->getExtendedObject()->getDocument());
        for (std::vector<App::DocumentObject*>::const_iterator it = links.begin(); it != links.end(); ++it) {
            ViewProvider* view = doc->getViewProvider(*it);
            if (view) 
                view->show();
        }
    }

    ViewProviderExtension::extensionShow();
    this->visible = true;
}

void ViewProviderGroupExtension::extensionHide(void) {

    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    if (!getExtendedViewProvider()->isRestoring() && this->visible) {

        auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();

        const std::vector<App::DocumentObject*> & links = group->Group.getValues();
        Gui::Document* doc = Application::Instance->getDocument(getExtendedViewProvider()->getObject()->getDocument());
        // doc pointer can be null in case the document is about to be destroyed
        // See https://forum.freecadweb.org/viewtopic.php?f=22&t=26797&p=218804#p218521
        if (doc) {
            for (std::vector<App::DocumentObject*>::const_iterator it = links.begin(); it != links.end(); ++it) {
                ViewProvider* view = doc->getViewProvider(*it);
                if (view)
                    view->hide();
            }
        }
    }

    ViewProviderExtension::extensionHide();
    this->visible = false;
}

bool ViewProviderGroupExtension::extensionOnDelete(const std::vector< std::string >& ) {

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
    // If the group is nonempty ask the user if he wants to delete its content
    if ( group->Group.getSize () ) {
        QMessageBox::StandardButton choice = 
            QMessageBox::question ( 0, QObject::tr ( "Delete group content?" ), 
                QObject::tr ( "The %1 is not empty, delete its content as well?")
                    .arg ( QString::fromUtf8 ( getExtendedViewProvider()->getObject()->Label.getValue () ) ), 
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );

        if ( choice == QMessageBox::Yes ) {
            Gui::Command::doCommand(Gui::Command::Doc,
                    "App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()"
                    ,getExtendedViewProvider()->getObject()->getDocument()->getName(), getExtendedViewProvider()->getObject()->getNameInDocument());
        }
    }
    return true;
}


namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGroupExtensionPython, Gui::ViewProviderGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderGroupExtension>;
}
