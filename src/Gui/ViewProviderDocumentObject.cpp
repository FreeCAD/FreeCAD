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
#include <App/DocumentObserver.h>
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
#include "SoFCUnifiedSelection.h"
#include "SoFCSelection.h"
#include "ViewParams.h"
#include "Tree.h"
#include <Gui/ViewProviderDocumentObjectPy.h>

FC_LOG_LEVEL_INIT("Gui",true,true)

using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderDocumentObject, Gui::ViewProvider)

ViewProviderDocumentObject::ViewProviderDocumentObject()
  : pcObject(0)
  , pcDocument(0)
{
    ADD_PROPERTY(DisplayMode,((long)0));
    ADD_PROPERTY(Visibility,(true));
    ADD_PROPERTY(ShowInTree,(true));

    ADD_PROPERTY(SelectionStyle,((long)0));
    static const char *SelectionStyleEnum[] = {"Shape","BoundBox",0};
    SelectionStyle.setEnums(SelectionStyleEnum);

    ADD_PROPERTY(Selectable,(true));
    Selectable.setValue(ViewParams::instance()->getEnableSelection());

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
    callExtension(&ViewProviderExtension::extensionStartRestoring);
}

void ViewProviderDocumentObject::finishRestoring()
{
    callExtension(&ViewProviderExtension::extensionFinishRestoring);
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

bool ViewProviderDocumentObject::removeDynamicProperty(const char* name)
{
    App::Property* prop = getDynamicPropertyByName(name);
    if(!prop || prop->testStatus(App::Property::LockDynamic))
        return false;

    // transactions of view providers are also managed in App::Document.
    App::DocumentObject* docobject = getObject();
    App::Document* document = docobject ? docobject->getDocument() : nullptr;
    if (document)
        document->addOrRemovePropertyOfObject(this, prop, false);

    return ViewProvider::removeDynamicProperty(name);
}

App::Property* ViewProviderDocumentObject::addDynamicProperty(
    const char* type, const char* name, const char* group, const char* doc,
    short attr, bool ro, bool hidden)
{
    auto prop = ViewProvider::addDynamicProperty(type,name,group,doc,attr,ro,hidden);
    if(prop) {
        // transactions of view providers are also managed in App::Document.
        App::DocumentObject* docobject = getObject();
        App::Document* document = docobject ? docobject->getDocument() : nullptr;
        if (document)
            document->addOrRemovePropertyOfObject(this, prop, true);
    }
    return prop;
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
        if (!Visibility.testStatus(App::Property::User1)
                && getObject() 
                && getObject()->Visibility.getValue()!=Visibility.getValue())
        {
            getObject()->Visibility.setValue(Visibility.getValue());
        }
    }
    else if (prop == &SelectionStyle || prop == &Selectable) {
        if(getRoot()->isOfType(SoFCSelectionRoot::getClassTypeId())) {
            auto root = static_cast<SoFCSelectionRoot*>(getRoot());
            if(Selectable.getValue()) {
                root->selectionStyle = SelectionStyle.getValue()
                    ? SoFCSelectionRoot::Box : SoFCSelectionRoot::Full;
            } else
                root->selectionStyle = SoFCSelectionRoot::Unpickable;
        }
        if(prop == &Selectable)
            setSelectable(Selectable.getValue());
    }

    if (pcDocument && !pcDocument->isModified() && testStatus(Gui::ViewStatus::TouchDocument)) {
        if (prop)
            FC_LOG(prop->getFullName() << " changed");
        pcDocument->setModified(true);
    }

    ViewProvider::onChanged(prop);
}

void ViewProviderDocumentObject::hide(void)
{
    ViewProvider::hide();
    // use this bit to check whether 'Visibility' must be adjusted
    if (Visibility.getValue() && Visibility.testStatus(App::Property::User2) == false) {
        Visibility.setStatus(App::Property::User2, true);
        Visibility.setValue(false);
        Visibility.setStatus(App::Property::User2, false);
    }
}

void ViewProviderDocumentObject::setModeSwitch() {
    if(getObject() && !TreeWidget::isObjectShowable(getObject())) {
        if(pcModeSwitch->whichChild.getValue()!=-1) {
            pcModeSwitch->whichChild = -1;
            callExtension(&ViewProviderExtension::extensionModeSwitchChange);
        }
        return;
    }
    ViewProvider::setModeSwitch();
}

void ViewProviderDocumentObject::show(void)
{
    ViewProvider::show();

    // use this bit to check whether 'Visibility' must be adjusted
    if (!Visibility.getValue() && Visibility.testStatus(App::Property::User2) == false) {
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

    // Disable object visibility syncing
    Base::ObjectStatusLocker<App::Property::Status,App::Property> lock2(App::Property::User1, &Visibility);

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
    callExtension(&ViewProviderExtension::extensionAttach,pcObj);
}

void ViewProviderDocumentObject::reattach(App::DocumentObject *pcObj) {
    callExtension(&ViewProviderExtension::extensionReattach,pcObj);
}

void ViewProviderDocumentObject::update(const App::Property* prop)
{
    // bypass view provider update to always allow changing visibility from
    // document object
    if(prop == &getObject()->Visibility) {
        if(!isRestoring() && Visibility.getValue()!=getObject()->Visibility.getValue())
            Visibility.setValue(!Visibility.getValue());
    } else {
        // Disable object visibility syncing
        Base::ObjectStatusLocker<App::Property::Status,App::Property>
            guard(App::Property::User1, &Visibility);
        ViewProvider::update(prop);
    }
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
    if(queryExtension(&ViewProviderExtension::extensionCanDropObjectEx,obj,owner,subname,elements))
        return true;
    if(obj && obj->getDocument()!=getObject()->getDocument())
        return false;
    return canDropObject(obj);
}

int ViewProviderDocumentObject::replaceObject(
        App::DocumentObject *oldObj, App::DocumentObject *newObj)
{
    if(!oldObj || !oldObj->getNameInDocument()
            || !newObj || !newObj->getNameInDocument())
    {
        FC_THROWM(Base::RuntimeError,"Invalid object");
    }
    
    auto obj = getObject();
    if(!obj || !obj->getNameInDocument())
        FC_THROWM(Base::RuntimeError,"View provider not attached");

    int res = ViewProvider::replaceObject(oldObj,newObj);
    if(res>=0)
        return res;

    std::vector<std::pair<App::DocumentObjectT, std::unique_ptr<App::Property> > > propChanges;
    std::vector<App::Property*> props;
    obj->getPropertyList(props);
    for(auto prop : props) {
        auto linkProp = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
        if(!linkProp)
            continue;
        std::unique_ptr<App::Property> copy(linkProp->CopyOnLinkReplace(obj, oldObj,newObj));
        if(!copy)
            continue;
        propChanges.emplace_back(prop,std::move(copy));
    }

    if(propChanges.empty())
        return 0;

    // Global search for affected links
    for(auto doc : App::GetApplication().getDocuments()) {
        for(auto o : doc->getObjects()) {
            if(o == obj)
                continue;
            std::vector<App::Property*> props;
            o->getPropertyList(props);
            for(auto prop : props) {
                auto linkProp = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
                if(!linkProp)
                    continue;
                std::unique_ptr<App::Property> copy(linkProp->CopyOnLinkReplace(obj,oldObj,newObj));
                if(!copy)
                    continue;
                propChanges.emplace_back(App::DocumentObjectT(prop),std::move(copy));
            }
        }
    }

    for(auto &v : propChanges) {
        auto prop = v.first.getProperty();
        if(prop)
            prop->Paste(*v.second.get());
    }
    return 1;
}

bool ViewProviderDocumentObject::showInTree() const {
    return ShowInTree.getValue();
}

bool ViewProviderDocumentObject::getElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    if(queryExtension(&ViewProviderExtension::extensionGetElementPicked,pp,subname))
        return true;

    SoPath* path = pp->getPath();
    int idx = -1;
    auto childRoot = getChildRoot();
    if(childRoot) 
        idx = path->findNode(childRoot);
    if(idx < 0) {
        subname = getElement(pp?pp->getDetail():0);
        return true;
    }

    if(idx+1<path->getLength()) {
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
    }
    return true;
}

bool ViewProviderDocumentObject::getDetailPath(
        const char *subname, SoFullPath *path, bool append, SoDetail *&det) const
{
    if(pcRoot->findChild(pcModeSwitch) < 0) {
        // this is possible in case of editing, where the switch node
        // of the linked view object is temporarily removed from its root
        // if(append)
        //     pPath->append(pcRoot);
        return false;
    }

    auto len = path->getLength();
    if(!append && len>=2)
        len -= 2;

    if(append) {
        path->append(pcRoot);
        path->append(pcModeSwitch);
    }
    if(queryExtension(&ViewProviderExtension::extensionGetDetailPath,subname,path,det))
        return true;

    const char *dot = 0;
    if(Data::ComplexGeoData::isMappedElement(subname) || (dot=strchr(subname,'.')) == 0) {
        det = getDetail(subname);
        return true;
    }

    auto obj = getObject();
    if(!obj || !obj->getNameInDocument()) return false;
    auto sobj = obj->getSubObject(std::string(subname,dot-subname+1).c_str());
    if(!sobj || !sobj->getNameInDocument()) return false;
    auto vp = Application::Instance->getViewProvider(sobj);
    if(!vp) return false;

    auto childRoot = getChildRoot();
    for(;;) {
        if(!childRoot) {
            // If no child root, then this view provider does not stack children
            // view provider under its own root, so we pop till before the root
            // node of this view provider.
            path->truncate(len);
        } else {
            // Do not account for our own visibility, we maybe called by a Link
            // that has independent visibility. Just make sure the child root node
            // is indeed a child of mode switch.
            if(pcModeSwitch->findChild(childRoot)<0)
                return false;
            path->append(childRoot);
        }
        if(path->getLength()) {
            SoNode * tail = path->getTail();
            const SoChildList * children = tail->getChildren();
            if(children && children->find(vp->getRoot())>=0)
                return vp->getDetailPath(dot+1,path,true,det);
        }
        if(childRoot) {
            // Can't find under child root, try again without it
            childRoot = 0;
        } else
            return false;
    }
}

void ViewProviderDocumentObject::onPropertyStatusChanged(
        const App::Property &prop, unsigned long oldStatus) 
{
    (void)oldStatus;
    if(!App::Document::isAnyRestoring() && pcObject && pcObject->getDocument())
        pcObject->getDocument()->signalChangePropertyEditor(*pcObject->getDocument(),prop);
}

ViewProviderDocumentObject *ViewProviderDocumentObject::getLinkedViewProvider(
        std::string *subname, bool recursive) const
{
    (void)subname;
    auto self = const_cast<ViewProviderDocumentObject*>(this);
    if(!pcObject || !pcObject->getNameInDocument())
        return self;
    auto linked = pcObject->getLinkedObject(recursive);
    if(!linked || linked == pcObject)
        return self;
    auto res = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(linked));
    if(!res)
        res = self;
    return res;
}

std::string ViewProviderDocumentObject::getFullName() const {
    if(pcObject)
        return pcObject->getFullName() + ".ViewObject";
    return std::string();
}

void ViewProviderDocumentObject::setSelectable(bool selectable)
{
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::ALL);
    sa.setSearchingAll(true);
    sa.setType(Gui::SoFCSelection::getClassTypeId());
    sa.apply(pcRoot);

    SoPathList & pathList = sa.getPaths();

    for (int i=0;i<pathList.getLength();i++) {
        SoFCSelection *selNode = dynamic_cast<SoFCSelection*>(pathList[i]->getTail());
        if (selectable) {
            if (selNode) {
                selNode->selectionMode = SoFCSelection::SEL_ON;
                selNode->highlightMode = SoFCSelection::AUTO;
            }
        }
        else {
            if (selNode) {
                selNode->selectionMode = SoFCSelection::SEL_OFF;
                selNode->highlightMode = SoFCSelection::OFF;
                selNode->selected = SoFCSelection::NOTSELECTED;
            }
        }
    }
}

Base::BoundBox3d ViewProviderDocumentObject::_getBoundingBox(
        const char *subname, const Base::Matrix4D *mat, bool transform,
        const View3DInventorViewer *viewer, int depth) const 
{
    if(!viewer) {
        viewer = getActiveViewer();
        if(!viewer) {
            FC_ERR("no view");
            return Base::BoundBox3d();
        }
    }

    App::DocumentObject *obj = getObject();
    if(!subname || !subname[0] || !obj) {
        if(obj) {
            auto subs = obj->getSubObjects(App::DocumentObject::GS_SELECT);
            if(subs.size()) {
                Base::BoundBox3d box;
                for(std::string &sub : subs) {
                    Base::Matrix4D smat;
                    if(mat)
                        smat = *mat;
                    App::DocumentObject *parent = 0;
                    std::string childName;
                    auto sobj = obj->resolve(sub.c_str(),&parent,&childName,0,0,&smat,transform,depth+1);
                    if(!sobj) 
                        continue;
                    int vis;
                    if(!parent || (vis=parent->isElementVisible(childName.c_str()))<0)
                        vis = sobj->Visibility.getValue()?1:0;
                    if(!vis)
                        continue;
                    auto vp = Application::Instance->getViewProvider(sobj);
                    if(!vp)
                        continue;
                    auto sbox = vp->getBoundingBox(0,&smat,false,viewer,depth+1);
                    if(sbox.IsValid())
                        box.Add(sbox);
                }
                return box;
            }
        }

        return ViewProvider::_getBoundingBox(0,mat,transform,viewer,depth);
    }

    Base::Matrix4D smat;
    if(mat)
        smat = *mat;
    const char *subelement = Data::ComplexGeoData::findElementName(subname);
    if(subelement == subname)
        return ViewProvider::_getBoundingBox(subname,&smat,false,viewer,depth+1);

    auto sobj = getObject()->getSubObject(subname,0,&smat,transform,depth);
    auto vp = Application::Instance->getViewProvider(sobj);
    if(!vp || vp==this)
        return Base::BoundBox3d();
    return vp->getBoundingBox(subelement, &smat,false,viewer,depth+1);
}

