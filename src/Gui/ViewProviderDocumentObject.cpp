/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QByteArray>
# include <qpixmap.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Tools.h>
#include <Base/Console.h>
#include <App/Material.h>
#include <App/DocumentObjectGroup.h>
#include <App/Origin.h>
#include "Application.h"
#include "Document.h"
#include "Selection.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "TaskView/TaskAppearance.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderExtension.h"
#include <Gui/ViewProviderDocumentObjectPy.h>


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderDocumentObject, Gui::ViewProvider)

ViewProviderDocumentObject::ViewProviderDocumentObject()
  : pcObject(0)
  , pcDocument(0)
{
    ADD_PROPERTY(DisplayMode,((long)0));
    ADD_PROPERTY(Visibility,(true));

    sPixmap = "Feature";
}

ViewProviderDocumentObject::~ViewProviderDocumentObject()
{
    // Make sure that the property class does not destruct our string list
    DisplayMode.setEnums(0);
}

void ViewProviderDocumentObject::getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>& vec) const
{
    vec.push_back(new Gui::TaskView::TaskAppearance());
}

void ViewProviderDocumentObject::startRestoring()
{
    hide();
}

void ViewProviderDocumentObject::finishRestoring()
{
}

bool ViewProviderDocumentObject::isAttachedToDocument() const
{
    return (!testStatus(Detach));
}

const char* ViewProviderDocumentObject::detachFromDocument()
{
    // here we can return an empty string since the object
    // name comes from the document object
    setStatus(Detach, true);
    return "";
}

void ViewProviderDocumentObject::onAboutToRemoveProperty(const char* prop)
{
    // transactions of view providers are also managed in App::Document.
    App::DocumentObject* docobject = getObject();
    App::Document* document = docobject ? docobject->getDocument() : nullptr;
    if (document)
        document->removePropertyOfObject(this, prop);
}

void ViewProviderDocumentObject::onBeforeChange(const App::Property* prop)
{
    if (isAttachedToDocument()) {
        App::DocumentObject* obj = getObject();
        App::Document* doc = obj ? obj->getDocument() : 0;
        if (doc) {
            onBeforeChangeProperty(doc, prop);
        }
    }
}

void ViewProviderDocumentObject::onChanged(const App::Property* prop)
{
    if (prop == &DisplayMode) {
        setActiveMode();
    }
    else if (prop == &Visibility) {
        // use this bit to check whether show() or hide() must be called
        if (Visibility.testStatus(App::Property::User2) == false) {
            Visibility.setStatus(App::Property::User2, true);
            Visibility.getValue() ? show() : hide();
            Visibility.setStatus(App::Property::User2, false);
        }
    }

    if (pcDocument)
        pcDocument->setModified(true);

    ViewProvider::onChanged(prop);
}

void ViewProviderDocumentObject::hide(void)
{
    // use this bit to check whether 'Visibility' must be adjusted
    if (Visibility.testStatus(App::Property::User2) == false) {
        Visibility.setStatus(App::Property::User2, true);
        Visibility.setValue(false);
        Visibility.setStatus(App::Property::User2, false);
    }
    ViewProvider::hide();
}

void ViewProviderDocumentObject::show(void)
{
    // use this bit to check whether 'Visibility' must be adjusted
    if (Visibility.testStatus(App::Property::User2) == false) {
        Visibility.setStatus(App::Property::User2, true);
        Visibility.setValue(true);
        Visibility.setStatus(App::Property::User2, false);
    }
    ViewProvider::show();
}

void ViewProviderDocumentObject::updateView()
{
    if(testStatus(ViewStatus::UpdatingView))
        return;

    Base::ObjectStatusLocker<ViewStatus,ViewProviderDocumentObject> lock(ViewStatus::UpdatingView,this);

    std::map<std::string, App::Property*> Map;
    pcObject->getPropertyMap(Map);

    // Hide the object temporarily to speed up the update
    bool vis = ViewProvider::isShow();
    if (vis) ViewProvider::hide();
    for (std::map<std::string, App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it) {
        updateData(it->second);
    }
    if (vis) ViewProvider::show();
}

void ViewProviderDocumentObject::attach(App::DocumentObject *pcObj)
{
    // save Object pointer
    pcObject = pcObj;

    // Retrieve the supported display modes of the view provider
    aDisplayModesArray = this->getDisplayModes();

    if (aDisplayModesArray.empty())
        aDisplayModesArray.push_back("");

    // We must collect the const char* of the strings and give it to PropertyEnumeration,
    // but we are still responsible for them, i.e. the property class must not delete the literals.
    for (std::vector<std::string>::iterator it = aDisplayModesArray.begin(); it != aDisplayModesArray.end(); ++it) {
        aDisplayEnumsArray.push_back( it->c_str() );
    }
    aDisplayEnumsArray.push_back(0); // null termination
    DisplayMode.setEnums(&(aDisplayEnumsArray[0]));

    // set the active mode
    const char* defmode = this->getDefaultDisplayMode();
    if (defmode)
        DisplayMode.setValue(defmode);

    //attach the extensions
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector)
        ext->extensionAttach(pcObj);
}

void ViewProviderDocumentObject::updateData(const App::Property* prop)
{
    ViewProvider::updateData(prop);
}

Gui::Document* ViewProviderDocumentObject::getDocument() const
{
    App::Document* pAppDoc = pcObject->getDocument();
    return Gui::Application::Instance->getDocument(pAppDoc);
}

Gui::MDIView* ViewProviderDocumentObject::getActiveView() const
{
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getActiveView();
}

Gui::MDIView* ViewProviderDocumentObject::getEditingView() const
{
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getEditingViewOfViewProvider(const_cast<ViewProviderDocumentObject*>(this));
}

Gui::MDIView* ViewProviderDocumentObject::getInventorView() const
{
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);

    Gui::MDIView* mdi = pGuiDoc->getEditingViewOfViewProvider(const_cast<ViewProviderDocumentObject*>(this));
    if (!mdi) {
        mdi = pGuiDoc->getViewOfViewProvider(const_cast<ViewProviderDocumentObject*>(this));
    }

    return mdi;
}

Gui::MDIView* ViewProviderDocumentObject::getViewOfNode(SoNode* node) const
{
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getViewOfNode(node);
}

SoNode* ViewProviderDocumentObject::findFrontRootOfType(const SoType& type) const
{
    // first get the document this object is part of and get its GUI counterpart
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);

    SoSearchAction searchAction;
    searchAction.setType(type);
    searchAction.setInterest(SoSearchAction::FIRST);

    // search in all view providers for the node type
    std::vector<App::DocumentObject*> obj = pAppDoc->getObjects();
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
        const ViewProvider* vp = pGuiDoc->getViewProvider(*it);
        // Ignore 'this' view provider. It could also happen that vp is 0, e.g. when
        // several objects have been added to the App::Document before notifying the
        // Gui::Document
        if (!vp || vp == this)
            continue;
        SoSeparator* front = vp->getFrontRoot();
        //if (front && front->getTypeId() == type)
        //    return front;
        if (front) {
            searchAction.apply(front);
            SoPath* path = searchAction.getPath();
            if (path)
                return path->getTail();
        }
    }

    return 0;
}

void ViewProviderDocumentObject::setActiveMode()
{
    if (DisplayMode.isValid()) {
        const char* mode = DisplayMode.getValueAsString();
        if (mode)
            setDisplayMode(mode);
    }
    if (!Visibility.getValue())
        ViewProvider::hide();
}

bool ViewProviderDocumentObject::canDelete(App::DocumentObject* obj) const
{
    Q_UNUSED(obj)
    if (getObject()->hasExtension(App::GroupExtension::getExtensionClassTypeId()))
        return true;
    if (getObject()->isDerivedFrom(App::Origin::getClassTypeId()))
        return true;
    return false;
}

PyObject* ViewProviderDocumentObject::getPyObject()
{
    if (!pyViewObject)
        pyViewObject = new ViewProviderDocumentObjectPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}
