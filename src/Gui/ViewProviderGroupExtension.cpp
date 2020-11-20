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
#include "MainWindow.h"
#include <Base/Tools.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GroupExtension.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Expression.h>
#include <Base/Console.h>
#include <QMessageBox>

using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderGroupExtension, Gui::ViewProviderExtension)

ViewProviderGroupExtension::ViewProviderGroupExtension()
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

void ViewProviderGroupExtension::extensionAttach(App::DocumentObject* pcObject)
{
    if(App::GeoFeatureGroupExtension::isNonGeoGroup(pcObject)) 
        getExtendedViewProvider()->SelectionStyle.setValue(1);
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
                        .arg(QString::fromLatin1(doc->getName()),
                             QString::fromLatin1(grp->getNameInDocument()),
                             QString::fromLatin1(obj->getNameInDocument()));

    Gui::Command::doCommand(Gui::Command::App, cmd.toUtf8());
}

void ViewProviderGroupExtension::extensionClaimChildren(std::vector<App::DocumentObject*> &children) const {

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();    
    const auto &objs = group->Group.getValues();
    children.insert(children.end(),objs.begin(),objs.end());
}

bool ViewProviderGroupExtension::extensionOnDelete(const std::vector< std::string >& ) {

    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
    // If the group is nonempty ask the user if he wants to delete its content
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

int ViewProviderGroupExtension::extensionCanReplaceObject(App::DocumentObject* oldValue,
                                                          App::DocumentObject* newValue)
{
    (void)newValue;
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
    if (!group->Group.find(oldValue->getNameInDocument()))
        return 0;
    return 1;
}

int ViewProviderGroupExtension::extensionReplaceObject(App::DocumentObject* oldValue,
                                                       App::DocumentObject* newValue)
{
    auto* group = getExtendedViewProvider()->getObject()->getExtensionByType<App::GroupExtension>();
    int idx = -1;
    group->Group.find(oldValue->getNameInDocument(), &idx);
    if (idx < 0)
        return 0;
    int idx2 = -1;
    group->Group.find(newValue->getNameInDocument(), &idx2);
    if (idx2 < 0)
        return -1;
    auto children = group->Group.getValues();
    children.erase(children.begin()+idx2);
    children.insert(children.begin()+idx, newValue);
    group->Group.setValues(children);
    return 1;
}

namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGroupExtensionPython, Gui::ViewProviderGroupExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderGroupExtension>;
}
