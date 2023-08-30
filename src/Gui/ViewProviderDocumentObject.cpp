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
# include <QAction>
# include <QMenu>
# include <Inventor/SoFullPath.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/misc/SoChildList.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include <Base/Tools.h>

#include "ViewProviderDocumentObjectPy.h"
#include "ActionFunction.h"
#include "Application.h"
#include "Command.h"
#include "Document.h"
#include "MDIView.h"
#include "SoFCUnifiedSelection.h"
#include "Tree.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderExtension.h"
#include "TaskView/TaskAppearance.h"


FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderDocumentObject, Gui::ViewProvider)

ViewProviderDocumentObject::ViewProviderDocumentObject()
{
    static const char *dogroup = "Display Options";
    static const char *sgroup = "Selection";

    ADD_PROPERTY_TYPE(DisplayMode, ((long)0), dogroup, App::Prop_None, "Set the display mode");
    ADD_PROPERTY_TYPE(Visibility, (true), dogroup, App::Prop_None, "Show the object in the 3d view");
    ADD_PROPERTY_TYPE(ShowInTree, (true), dogroup, App::Prop_None, "Show the object in the tree view");

    ADD_PROPERTY_TYPE(SelectionStyle, ((long)0), sgroup, App::Prop_None, "Set the object selection style");
    static const char *SelectionStyleEnum[] = {"Shape","BoundBox",nullptr};
    SelectionStyle.setEnums(SelectionStyleEnum);

    static const char* OnTopEnum[]= {"Disabled","Enabled","Object","Element",nullptr};
    ADD_PROPERTY_TYPE(OnTopWhenSelected,((long int)0), sgroup, App::Prop_None,
            "Enabled: Display the object on top of any other object when selected\n"
            "Object: On top only if the whole object is selected\n"
            "Element: On top only if some sub-element of the object is selected");
    OnTopWhenSelected.setEnums(OnTopEnum);

    sPixmap = "Feature";
}

ViewProviderDocumentObject::~ViewProviderDocumentObject()
{
    // Make sure that the property class does not destruct our string list
    DisplayMode.setContainer(nullptr);
    DisplayMode.setEnums(nullptr);
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
        App::Document* doc = obj ? obj->getDocument() : nullptr;
        if (doc) {
            onBeforeChangeProperty(doc, prop);
        }
    }

    ViewProvider::onBeforeChange(prop);
}

void ViewProviderDocumentObject::onChanged(const App::Property* prop)
{
    if (prop == &DisplayMode) {
        setActiveMode();
    }
    else if (prop == &Visibility) {
        // use this bit to check whether show() or hide() must be called
        if (!Visibility.testStatus(App::Property::User2)) {
            Visibility.setStatus(App::Property::User2, true);
            Visibility.getValue() ? show() : hide();
            Visibility.setStatus(App::Property::User2, false);
        }
        if (!Visibility.testStatus(App::Property::User1)
                && getObject()
                && getObject()->Visibility.getValue()!=Visibility.getValue())
        {
            // Changing the visibility of a document object will automatically set
            // the document modified but if the 'TouchDocument' flag is not set then
            // this is undesired behaviour. So, if this change marks the document as
            // modified then it must be be reversed.
            if (!testStatus(Gui::ViewStatus::TouchDocument)) {
                // Note: reverting document modified status like that is not
                // appropriate because we can't tell if there is any other
                // property being changed due to the change of Visibility here.
                // Temporary setting the Visibility property as 'NoModify' is
                // the proper way.
                Base::ObjectStatusLocker<App::Property::Status,App::Property> guard(
                        App::Property::NoModify, &Visibility);
                // bool mod = false;
                // if (pcDocument)
                //     mod = pcDocument->isModified();
                getObject()->Visibility.setValue(Visibility.getValue());
                // if (pcDocument)
                //     pcDocument->setModified(mod);
            }
            else {
                getObject()->Visibility.setValue(Visibility.getValue());
            }
        }
    }
    else if (prop == &SelectionStyle) {
        if(getRoot()->isOfType(SoFCSelectionRoot::getClassTypeId())) {
            static_cast<SoFCSelectionRoot*>(getRoot())->selectionStyle = SelectionStyle.getValue()
                ? SoFCSelectionRoot::Box : SoFCSelectionRoot::Full;
        }
    }

    if (prop && !prop->testStatus(App::Property::NoModify)
             && pcDocument
             && !pcDocument->isModified()
             && testStatus(Gui::ViewStatus::TouchDocument)) {
        if (prop)
            FC_LOG(prop->getFullName() << " changed");
        pcDocument->setModified(true);
    }

    ViewProvider::onChanged(prop);
}

void ViewProviderDocumentObject::hide()
{
    ViewProvider::hide();
    // use this bit to check whether 'Visibility' must be adjusted
    if (!Visibility.testStatus(App::Property::User2)) {
        Visibility.setStatus(App::Property::User2, true);
        Visibility.setValue(false);
        Visibility.setStatus(App::Property::User2, false);
    }
}

bool ViewProviderDocumentObject::isShowable() const
{
    return _Showable;
}

void ViewProviderDocumentObject::setShowable(bool enable)
{
    if (_Showable == enable)
        return;

    _Showable = enable;
    int which = getModeSwitch()->whichChild.getValue();
    if (_Showable && which == -1 && Visibility.getValue()) {
        setModeSwitch();
    }
    else if (!_Showable) {
        if (which >= 0)
            ViewProvider::hide();
    }
}

void ViewProviderDocumentObject::startDefaultEditMode()
{
    QString text = QObject::tr("Edit %1").arg(QString::fromUtf8(getObject()->Label.getValue()));
    Gui::Command::openCommand(text.toUtf8());

    Gui::Document* document = this->getDocument();
    if (document) {
        document->setEdit(this, ViewProvider::Default);
    }
}

void ViewProviderDocumentObject::addDefaultAction(QMenu* menu, const QString& text)
{
    QAction* act = menu->addAction(text);
    act->setData(QVariant((int)ViewProvider::Default));
    auto func = new Gui::ActionFunction(menu);
    func->trigger(act, [this](){
        this->startDefaultEditMode();
    });
}

void ViewProviderDocumentObject::setModeSwitch() {
    if(isShowable())
        ViewProvider::setModeSwitch();
}

void ViewProviderDocumentObject::show()
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
    if (!Visibility.testStatus(App::Property::User2)) {
        Visibility.setStatus(App::Property::User2, true);
        Visibility.setValue(true);
        Visibility.setStatus(App::Property::User2, false);
    }
}

const char* ViewProviderDocumentObject::getTransactionText() const
{
    return QT_TRANSLATE_NOOP("Command", "Edit");
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
    for (const auto & it : Map) {
        updateData(it.second);
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
        aDisplayModesArray.emplace_back("");

    // We must collect the const char* of the strings and give it to PropertyEnumeration,
    // but we are still responsible for them, i.e. the property class must not delete the literals.
    //for (auto it = aDisplayModesArray.begin(); it != aDisplayModesArray.end(); ++it) {
    for (const auto & it : aDisplayModesArray) {
        aDisplayEnumsArray.push_back( it.c_str() );
    }
    aDisplayEnumsArray.push_back(nullptr); // null termination
    DisplayMode.setEnums(&(aDisplayEnumsArray[0]));

    if(!isRestoring()) {
        // set the active mode
        const char* defmode = this->getDefaultDisplayMode();
        if (defmode)
            DisplayMode.setValue(defmode);
    }

    //attach the extensions
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector)
        ext->extensionAttach(pcObj);
}

void ViewProviderDocumentObject::reattach(App::DocumentObject *pcObj) {
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for (Gui::ViewProviderExtension* ext : vector)
        ext->extensionReattach(pcObj);
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
    if (pcDocument) {
        return pcDocument;
    }
    else {
        App::Document* pAppDoc = pcObject->getDocument();
        return Gui::Application::Instance->getDocument(pAppDoc);
    }
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
        return nullptr;
    // first get the document this object is part of and get its GUI counterpart
    App::Document* pAppDoc = pcObject->getDocument();
    Gui::Document* pGuiDoc = Gui::Application::Instance->getDocument(pAppDoc);

    SoSearchAction searchAction;
    searchAction.setType(type);
    searchAction.setInterest(SoSearchAction::FIRST);

    // search in all view providers for the node type
    std::vector<App::DocumentObject*> obj = pAppDoc->getObjects();
    for (auto & it : obj) {
        const ViewProvider* vp = pGuiDoc->getViewProvider(it);
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

    return nullptr;
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
    if(!isSelectable())
        return false;
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
    if(!vp)
        return false;
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
        det = nullptr;
    }

    const char *dot = strchr(subname,'.');
    if(!dot)
        return false;
    auto obj = getObject();
    if(!obj || !obj->getNameInDocument())
        return false;
    auto sobj = obj->getSubObject(std::string(subname,dot-subname+1).c_str());
    if(!sobj)
        return false;
    auto vp = Application::Instance->getViewProvider(sobj);
    if(!vp)
        return false;

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
    return std::string("?");
}
