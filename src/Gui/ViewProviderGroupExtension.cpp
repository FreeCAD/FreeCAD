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
#include <QMessageBox>

using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderGroupExtension, Gui::ViewProviderExtension)

ViewProviderGroupExtension::ViewProviderGroupExtension()  : visible(false)
{
    initExtension(ViewProviderGroupExtension::getClassTypeId());
    
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
    
    //we cannot drop anything into the group.  We need to find the correct App extension to ask 
    //if this is a supported type, there should only be one
    auto vector = getExtendedViewProvider()->getObject()->getExtensionsDerivedFromType<App::GroupExtension>();
    assert(vector.size() == 1);
    if(vector[0]->allowObject(obj)) 
        return true;

    return false;
    
}

void ViewProviderGroupExtension::extensionDropObject(App::DocumentObject* obj) {
    
    Gui::Command::doCommand(Gui::Command::Doc,"App.getDocument(\"%s\").getObject(\"%s\").addObject("
        "App.getDocument(\"%s\").getObject(\"%s\"))",
        getExtendedViewProvider()->getObject()->getDocument()->getName(), getExtendedViewProvider()->getObject()->getNameInDocument(), 
        obj->getDocument()->getName(), obj->getNameInDocument() );
}

std::vector< App::DocumentObject* > ViewProviderGroupExtension::extensionClaimChildren(void) const {
    
    auto ext = getExtendedViewProvider()->getObject()->getExtensionsDerivedFromType<App::GroupExtension>();
    assert(ext.size() == 1);
    
    return std::vector<App::DocumentObject*>(ext.front()->Group.getValues());
}


void ViewProviderGroupExtension::extensionShow(void) {
   
    // when reading the Visibility property from file then do not hide the
    // objects of this group because they have stored their visibility status, too
    if (!getExtendedViewProvider()->Visibility.testStatus(App::Property::User1) && !this->visible) {
        auto ext = getExtendedViewProvider()->getObject()->getExtensionsDerivedFromType<App::GroupExtension>();
        assert(ext.size() == 1);
    
        App::GroupExtension* group = ext.front();
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
    if (!getExtendedViewProvider()->Visibility.testStatus(App::Property::User1) && this->visible) {
        
        auto ext = getExtendedViewProvider()->getObject()->getExtensionsDerivedFromType<App::GroupExtension>();
        assert(ext.size() == 1);
    
        App::GroupExtension* group = ext.front();
        const std::vector<App::DocumentObject*> & links = group->Group.getValues();
        Gui::Document* doc = Application::Instance->getDocument(getExtendedViewProvider()->getObject()->getDocument());
        for (std::vector<App::DocumentObject*>::const_iterator it = links.begin(); it != links.end(); ++it) {
            ViewProvider* view = doc->getViewProvider(*it);
            if (view) 
                view->hide();
        }
    }

    ViewProviderExtension::extensionHide();
    this->visible = false;
}

bool ViewProviderGroupExtension::extensionOnDelete(const std::vector< std::string >& vec) {
    
    auto ext = getExtendedViewProvider()->getObject()->getExtensionsDerivedFromType<App::GroupExtension>();
    assert(ext.size() == 1);
    
    App::GroupExtension *group = ext.front();
    // If the group is nonempty ask the user if he wants to delete it's content
    if ( group->Group.getSize () ) {
        QMessageBox::StandardButton choice = 
            QMessageBox::question ( 0, QObject::tr ( "Delete group content?" ), 
                QObject::tr ( "The %1 is not empty, delete it's content as well?")
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

void ViewProviderGroupExtension::extensionRestore(Base::XMLReader& reader) {
    
    getExtendedViewProvider()->Visibility.setStatus(App::Property::User1, true); // tmp. set
    ViewProviderExtension::extensionRestore(reader);
    getExtendedViewProvider()->Visibility.setStatus(App::Property::User1, false); // unset
}

