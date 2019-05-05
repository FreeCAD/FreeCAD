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
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/SoFullPath.h>
# include <Inventor/misc/SoChildList.h>
# include <Inventor/details/SoDetail.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/BoundBox.h>
#include <App/Material.h>
#include <App/DocumentObjectGroup.h>
#include <App/Origin.h>
#include "Application.h"
#include "Document.h"
#include "Selection.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "TaskView/TaskAppearance.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderExtension.h"
#include "Tree.h"
#include <Gui/ViewProviderDocumentObjectPy.h>


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderDocumentObject, Gui::ViewProvider)

ViewProviderDocumentObject::ViewProviderDocumentObject()
  : pcObject(0)
{
    ADD_PROPERTY(DisplayMode,((long)0));
    ADD_PROPERTY(Visibility,(true));
    ADD_PROPERTY(ShowInTree,(true));

    static const char* OnTopEnum[]= {"Disabled","Enabled","Object","Element",NULL};
    ADD_PROPERTY(OnTopWhenSelected,((long int)0));
    ADD_PROPERTY_TYPE(OnTopWhenSelected,((long int)0), "Base", App::Prop_None, 
            "Enabled: Display the object on top of any other object when selected\n"
            "Object: On top only if the whole object is selected\n"
            "Element: On top only if some sub-element of the object is selected");
    OnTopWhenSelected.setEnums(OnTopEnum);

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
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for(Gui::ViewProviderExtension* ext : vector)
        ext->extensionStartRestoring();
}

void ViewProviderDocumentObject::finishRestoring()
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for(Gui::ViewProviderExtension* ext : vector)
        ext->extensionFinishRestoring();
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
        if(getObject() && getObject()->Visibility.getValue()!=Visibility.getValue())
            getObject()->Visibility.setValue(Visibility.getValue());
    }

    ViewProvider::onChanged(prop);
}

void ViewProviderDocumentObject::hide(void)
{
    ViewProvider::hide();
    // use this bit to check whether 'Visibility' must be adjusted
    if (Visibility.testStatus(App::Property::User2) == false) {
        Visibility.setStatus(App::Property::User2, true);
        Visibility.setValue(false);
        Visibility.setStatus(App::Property::User2, false);
    }
}

void ViewProviderDocumentObject::show(void)
{
    if(TreeWidget::isObjectShowable(getObject()))
        ViewProvider::show();
    else {
        Visibility.setValue(false);
        if(getObject())
            getObject()->Visibility.setValue(false);
        return;
    }

    // use this bit to check whether 'Visibility' must be adjusted
    if (Visibility.testStatus(App::Property::User2) == false) {
        Visibility.setStatus(App::Property::User2, true);
        Visibility.setValue(true);
        Visibility.setStatus(App::Property::User2, false);
    }
}

void ViewProviderDocumentObject::updateView()
{
    if(!pcObject || testStatus(ViewStatus::UpdatingView))
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
    if (vis && Visibility.getValue()) ViewProvider::show();
}

void ViewProviderDocumentObject::attach(App::DocumentObject *pcObj)
{
    // save Object pointer
    pcObject = pcObj;

    if(pcObj && pcObj->getNameInDocument() &&
       Visibility.getValue()!=pcObj->Visibility.getValue())
        pcObj->Visibility.setValue(Visibility.getValue());

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

void ViewProviderDocumentObject::update(const App::Property* prop)
{
    // bypass view provider update to always allow changing visibility from
    // document object
    if(prop == &getObject()->Visibility) {
        if(!isRestoring() && Visibility.getValue()!=getObject()->Visibility.getValue())
            Visibility.setValue(!Visibility.getValue());
    }else
        ViewProvider::update(prop);
}

Gui::Document* ViewProviderDocumentObject::getDocument() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    return Gui::Application::Instance->getDocument(pAppDoc);
}

Gui::MDIView* ViewProviderDocumentObject::getActiveView() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getActiveView();
}

Gui::MDIView* ViewProviderDocumentObject::getEditingView() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getEditingViewOfViewProvider(const_cast<ViewProviderDocumentObject*>(this));
}

Gui::MDIView* ViewProviderDocumentObject::getInventorView() const
{
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
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
    if(!pcObject)
        throw Base::RuntimeError("View provider detached");
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);
    return pGuiDoc->getViewOfNode(node);
}

SoNode* ViewProviderDocumentObject::findFrontRootOfType(const SoType& type) const
{
    if(!pcObject)
        return 0;
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

bool ViewProviderDocumentObject::canDropObjectEx(App::DocumentObject* obj, App::DocumentObject *owner, 
        const char *subname, const std::vector<std::string> &elements) const
{
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for(Gui::ViewProviderExtension* ext : vector){
        if(ext->extensionCanDropObjectEx(obj,owner,subname,elements))
            return true;
    }
    if(obj && obj->getDocument()!=getObject()->getDocument())
        return false;
    return canDropObject(obj);
}

bool ViewProviderDocumentObject::showInTree() const {
    return ShowInTree.getValue();
}

bool ViewProviderDocumentObject::getElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    if(!isSelectable()) return false;
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for(Gui::ViewProviderExtension* ext : vector)
        if(ext->extensionGetElementPicked(pp,subname))
            return true;

    auto childRoot = getChildRoot();
    int idx;
    if(!childRoot || 
       (idx=pcModeSwitch->whichChild.getValue())<0 ||
       pcModeSwitch->getChild(idx)!=childRoot)
    {
        return ViewProvider::getElementPicked(pp,subname);
    }

    SoPath* path = pp->getPath();
    idx = path->findNode(childRoot);
    if(idx<0 || idx+1>=path->getLength())
        return false;
    auto vp = getDocument()->getViewProvider(path->getNode(idx+1));
    if(!vp) return false;
    auto obj = vp->getObject();
    if(!obj || !obj->getNameInDocument())
        return false;
    std::ostringstream str;
    str << obj->getNameInDocument() << '.';
    if(vp->getElementPicked(pp,subname))
        str << subname;
    subname = str.str();
    return true;
}

bool ViewProviderDocumentObject::getDetailPath(const char *subname, SoFullPath *path, bool append, SoDetail *&det) const
{
    auto len = path->getLength();
    if(!append && len>=2)
        len -= 2;
    if(ViewProvider::getDetailPath(subname,path,append,det)) {
        if(det || !subname || !*subname)
            return true;
    }

    if(det) {
        delete det;
        det = 0;
    }

    const char *dot = strchr(subname,'.');
    if(!dot) return false;
    auto obj = getObject();
    if(!obj || !obj->getNameInDocument()) return false;
    auto sobj = obj->getSubObject(std::string(subname,dot-subname+1).c_str());
    if(!sobj) return false;
    auto vp = Application::Instance->getViewProvider(sobj);
    if(!vp) return false;

    auto childRoot = getChildRoot();
    if(!childRoot) 
        path->truncate(len);
    else {
        auto idx = pcModeSwitch->whichChild.getValue();
        if(idx < 0 || pcModeSwitch->getChild(idx)!=childRoot)
            return false;
        path->append(childRoot);
    }
    bool ret = false;
    if(path->getLength()) {
        SoNode * tail = path->getTail();
        const SoChildList * children = tail->getChildren();
        if(children && children->find(vp->getRoot())>=0)
            ret = vp->getDetailPath(dot+1,path,true,det);
    }
    return ret;
}

void ViewProviderDocumentObject::onPropertyStatusChanged(
        const App::Property &prop, unsigned long oldStatus) 
{
    (void)oldStatus;
    if(!App::Document::isAnyRestoring() && pcObject && pcObject->getDocument())
        pcObject->getDocument()->signalChangePropertyEditor(*pcObject->getDocument(),prop);
}
