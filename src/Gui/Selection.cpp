/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <boost/algorithm/string/predicate.hpp>
# include <QApplication>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "Selection.h"
#include "SelectionObject.h"
#include "Application.h"
#include "Document.h"
#include "Macro.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "SelectionFilter.h"
#include "SelectionFilterPy.h"
#include "SelectionObserverPython.h"
#include "Tree.h"
#include "ViewProviderDocumentObject.h"


FC_LOG_LEVEL_INIT("Selection",false,true,true)

using namespace Gui;
using namespace std;
namespace sp = std::placeholders;

SelectionGateFilterExternal::SelectionGateFilterExternal(const char *docName, const char *objName) {
    if(docName) {
        DocName = docName;
        if(objName)
            ObjName = objName;
    }
}

bool SelectionGateFilterExternal::allow(App::Document *doc ,App::DocumentObject *obj, const char*) {
    if(!doc || !obj)
        return true;
    if(!DocName.empty() && doc->getName()!=DocName)
        notAllowedReason = "Cannot select external object";
    else if(!ObjName.empty() && ObjName==obj->getNameInDocument())
        notAllowedReason = "Cannot select self";
    else
        return true;
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////

SelectionObserver::SelectionObserver(bool attach, ResolveMode resolve)
    : resolve(resolve)
    , blockedSelection(false)
{
    if (attach)
        attachSelection();
}

SelectionObserver::SelectionObserver(const ViewProviderDocumentObject *vp, bool attach, ResolveMode resolve)
    : resolve(resolve)
    , blockedSelection(false)
{
    if (vp && vp->getObject() && vp->getObject()->getDocument()) {
        filterDocName = vp->getObject()->getDocument()->getName();
        filterObjName = vp->getObject()->getNameInDocument();
    }
    if (attach)
        attachSelection();
}


SelectionObserver::~SelectionObserver()
{
    detachSelection();
}

bool SelectionObserver::blockSelection(bool block)
{
    bool ok = blockedSelection;
    blockedSelection = block;
    return ok;
}

bool SelectionObserver::isSelectionBlocked() const
{
    return blockedSelection;
}

bool SelectionObserver::isSelectionAttached() const
{
    return connectSelection.connected();
}

void SelectionObserver::attachSelection()
{
    if (!connectSelection.connected()) {
        bool newStyle = (resolve >= ResolveMode::NewStyleElement);
        bool oldStyle = (resolve == ResolveMode::OldStyleElement);
        auto &signal = newStyle ? Selection().signalSelectionChanged3 :
                       oldStyle ? Selection().signalSelectionChanged2 :
                                  Selection().signalSelectionChanged  ;
        //NOLINTBEGIN
        connectSelection = signal.connect(std::bind
            (&SelectionObserver::_onSelectionChanged, this, sp::_1));
        //NOLINTEND

        if (!filterDocName.empty()) {
            Selection().addSelectionGate(
                    new SelectionGateFilterExternal(filterDocName.c_str(),filterObjName.c_str()));
        }
    }
}

void SelectionObserver::_onSelectionChanged(const SelectionChanges& msg) {
    try {
        if (blockedSelection)
            return;
        onSelectionChanged(msg);
    } catch (Base::Exception &e) {
        e.ReportException();
        FC_ERR("Unhandled Base::Exception caught in selection observer: ");
    } catch (std::exception &e) {
        FC_ERR("Unhandled std::exception caught in selection observer: " << e.what());
    } catch (...) {
        FC_ERR("Unhandled unknown exception caught in selection observer");
    }
}

void SelectionObserver::detachSelection()
{
    if (connectSelection.connected()) {
        connectSelection.disconnect();
        if (!filterDocName.empty())
            Selection().rmvSelectionGate();
    }
}

// -------------------------------------------

bool SelectionSingleton::hasSelection() const
{
    return !_SelList.empty();
}

bool SelectionSingleton::hasPreselection() const {
    return !CurrentPreselection.Object.getObjectName().empty();
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getCompleteSelection(ResolveMode resolve) const
{
    return getSelection("*", resolve);
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getSelection(const char* pDocName, ResolveMode resolve, bool single) const
{
    std::vector<SelObj> temp;
    if (single)
        temp.reserve(1);
    SelObj tempSelObj;

    App::Document *pcDoc = nullptr;
    if(!pDocName || strcmp(pDocName,"*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc)
            return temp;
    }

    std::map<App::DocumentObject*,std::set<std::string> > objMap;

    for(auto &sel : _SelList) {
        if (!sel.pDoc)
            continue;
        const char *subelement = nullptr;
        auto obj = getObjectOfType(sel, App::DocumentObject::getClassTypeId(), resolve, &subelement);
        if (!obj || (pcDoc && sel.pObject->getDocument() != pcDoc))
            continue;

        // In case we are resolving objects, make sure no duplicates
        if (resolve != ResolveMode::NoResolve && !objMap[obj].insert(std::string(subelement ? subelement : "")).second)
            continue;

        if (single && !temp.empty()) {
            temp.clear();
            break;
        }

        tempSelObj.DocName  = obj->getDocument()->getName();
        tempSelObj.FeatName = obj->getNameInDocument();
        tempSelObj.SubName = subelement;
        tempSelObj.TypeName = obj->getTypeId().getName();
        tempSelObj.pObject  = obj;
        tempSelObj.pResolvedObject  = sel.pResolvedObject;
        tempSelObj.pDoc     = obj->getDocument();
        tempSelObj.x        = sel.x;
        tempSelObj.y        = sel.y;
        tempSelObj.z        = sel.z;

        temp.push_back(tempSelObj);
    }

    return temp;
}

bool SelectionSingleton::hasSelection(const char* doc, ResolveMode resolve) const
{
    App::Document *pcDoc = nullptr;
    if (!doc || strcmp(doc,"*") != 0) {
        pcDoc = getDocument(doc);
        if (!pcDoc)
            return false;
    }
    for(auto &sel : _SelList) {
        if (!sel.pDoc)
            continue;
        auto obj = getObjectOfType(sel, App::DocumentObject::getClassTypeId(), resolve);
        if (obj && (!pcDoc || sel.pObject->getDocument()==pcDoc)) {
            return true;
        }
    }

    return false;
}

bool SelectionSingleton::hasSubSelection(const char* doc, bool subElement) const
{
    App::Document *pcDoc = nullptr;
    if(!doc || strcmp(doc,"*")!=0) {
        pcDoc = getDocument(doc);
        if (!pcDoc)
            return false;
    }
    for(auto &sel : _SelList) {
        if(pcDoc && pcDoc != sel.pDoc)
            continue;
        if(sel.SubName.empty())
            continue;
        if(subElement && sel.SubName.back()!='.')
            return true;
        if(sel.pObject != sel.pResolvedObject)
            return true;
    }

    return false;
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getPickedList(const char* pDocName) const
{
    std::vector<SelObj> temp;
    SelObj tempSelObj;

    App::Document *pcDoc = nullptr;
    if(!pDocName || strcmp(pDocName,"*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc)
            return temp;
    }

    for(std::list<_SelObj>::const_iterator It = _PickedList.begin();It != _PickedList.end();++It) {
        if (!pcDoc || It->pDoc == pcDoc) {
            tempSelObj.DocName  = It->DocName.c_str();
            tempSelObj.FeatName = It->FeatName.c_str();
            tempSelObj.SubName  = It->SubName.c_str();
            tempSelObj.TypeName = It->TypeName.c_str();
            tempSelObj.pObject  = It->pObject;
            tempSelObj.pResolvedObject  = It->pResolvedObject;
            tempSelObj.pDoc     = It->pDoc;
            tempSelObj.x        = It->x;
            tempSelObj.y        = It->y;
            tempSelObj.z        = It->z;
            temp.push_back(tempSelObj);
        }
    }

    return temp;
}

std::vector<SelectionObject> SelectionSingleton::getSelectionEx(const char* pDocName, Base::Type typeId,
                                                                ResolveMode resolve, bool single) const
{
    return getObjectList(pDocName, typeId, _SelList, resolve, single);
}

std::vector<SelectionObject> SelectionSingleton::getPickedListEx(const char* pDocName, Base::Type typeId) const
{
    return getObjectList(pDocName, typeId, _PickedList, ResolveMode::NoResolve);
}

std::vector<SelectionObject> SelectionSingleton::getObjectList(const char* pDocName, Base::Type typeId,
                                                               std::list<_SelObj> &objList,
                                                               ResolveMode resolve, bool single) const
{
    std::vector<SelectionObject> temp;
    if (single)
        temp.reserve(1);
    std::map<App::DocumentObject*,size_t> SortMap;

    // check the type
    if (typeId == Base::Type::badType())
        return temp;

    App::Document *pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName,"*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc)
            return temp;
    }

    for (auto &sel : objList) {
        if(!sel.pDoc)
            continue;
        const char *subelement = nullptr;
        auto obj = getObjectOfType(sel,typeId,resolve,&subelement);
        if (!obj || (pcDoc && sel.pObject->getDocument() != pcDoc))
            continue;
        auto it = SortMap.find(obj);
        if(it!=SortMap.end()) {
            // only add sub-element
            if (subelement && *subelement) {
                if (resolve != ResolveMode::NoResolve && !temp[it->second]._SubNameSet.insert(subelement).second)
                    continue;
                temp[it->second].SubNames.emplace_back(subelement);
                temp[it->second].SelPoses.emplace_back(sel.x,sel.y,sel.z);
            }
        }
        else {
            if (single && !temp.empty()) {
                temp.clear();
                break;
            }
            // create a new entry
            temp.emplace_back(obj);
            if (subelement && *subelement) {
                temp.back().SubNames.emplace_back(subelement);
                temp.back().SelPoses.emplace_back(sel.x,sel.y,sel.z);
                if (resolve != ResolveMode::NoResolve)
                    temp.back()._SubNameSet.insert(subelement);
            }
            SortMap.insert(std::make_pair(obj,temp.size()-1));
        }
    }

    return temp;
}

bool SelectionSingleton::needPickedList() const
{
    return _needPickedList;
}

void SelectionSingleton::enablePickedList(bool enable)
{
    if(enable != _needPickedList) {
        _needPickedList = enable;
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }
}

void SelectionSingleton::notify(SelectionChanges &&Chng)
{
    if(Notifying) {
        NotificationQueue.push_back(std::move(Chng));
        return;
    }
    Base::FlagToggler<bool> flag(Notifying);
    NotificationQueue.push_back(std::move(Chng));
    while(!NotificationQueue.empty()) {
        const auto &msg = NotificationQueue.front();
        bool notify;
        switch(msg.Type) {
        case SelectionChanges::AddSelection:
            notify = isSelected(msg.pDocName, msg.pObjectName, msg.pSubName, ResolveMode::NoResolve);
            break;
        case SelectionChanges::RmvSelection:
            notify = !isSelected(msg.pDocName, msg.pObjectName, msg.pSubName, ResolveMode::NoResolve);
            break;
        case SelectionChanges::SetPreselect:
            notify = CurrentPreselection.Type==SelectionChanges::SetPreselect
                && CurrentPreselection.Object == msg.Object;
            break;
        case SelectionChanges::RmvPreselect:
            notify = CurrentPreselection.Type==SelectionChanges::ClrSelection;
            break;
        default:
            notify = true;
        }
        if(notify) {
            Notify(msg);
            try {
                signalSelectionChanged(msg);
            }
            catch (const boost::exception&) {
                // reported by code analyzers
                Base::Console().Warning("notify: Unexpected boost exception\n");
            }
        }
        NotificationQueue.pop_front();
    }
}

bool SelectionSingleton::hasPickedList() const
{
    return !_PickedList.empty();
}

int SelectionSingleton::getAsPropertyLinkSubList(App::PropertyLinkSubList &prop) const
{
    std::vector<Gui::SelectionObject> sel = this->getSelectionEx();
    std::vector<App::DocumentObject*> objs; objs.reserve(sel.size() * 2);
    std::vector<std::string> subs; subs.reserve(sel.size()*2);
    for (auto & selitem : sel) {
        App::DocumentObject* obj = selitem.getObject();
        const std::vector<std::string> &subnames = selitem.getSubNames();

        //whole object is selected
        if (subnames.empty()){
            objs.push_back(obj);
            subs.emplace_back();
        }
        else {
            for (const auto & subname : subnames) {
                objs.push_back(obj);
                subs.push_back(subname);
            }
        }
    }
    assert(objs.size()==subs.size());
    prop.setValues(objs, subs);
    return objs.size();
}

App::DocumentObject *SelectionSingleton::getObjectOfType(_SelObj &sel, Base::Type typeId,
                                                         ResolveMode resolve, const char **subelement)
{
    auto obj = sel.pObject;
    if(!obj || !obj->getNameInDocument())
        return nullptr;
    const char *subname = sel.SubName.c_str();
    if (resolve != ResolveMode::NoResolve) {
        obj = sel.pResolvedObject;
        if (resolve == ResolveMode::NewStyleElement && !sel.elementName.first.empty())
            subname = sel.elementName.first.c_str();
        else
            subname = sel.elementName.second.c_str();
    }

    if (!obj)
        return nullptr;

    if (!obj->isDerivedFrom(typeId) && (resolve != ResolveMode::FollowLink || !obj->getLinkedObject(true)->isDerivedFrom(typeId)))
        return nullptr;

    if (subelement)
        *subelement = subname;

    return obj;
}

vector<App::DocumentObject*> SelectionSingleton::getObjectsOfType(const Base::Type& typeId, const char* pDocName, ResolveMode resolve) const
{
    std::vector<App::DocumentObject*> temp;

    App::Document *pcDoc = nullptr;
    if (!pDocName || strcmp(pDocName,"*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc)
            return temp;
    }

    std::set<App::DocumentObject*> objs;
    for(auto &sel : _SelList) {
        if(pcDoc && pcDoc!=sel.pDoc) continue;
        App::DocumentObject *pObject = getObjectOfType(sel, typeId, resolve);
        if (pObject) {
            auto ret = objs.insert(pObject);
            if(ret.second)
                temp.push_back(pObject);
        }
    }

    return temp;
}

std::vector<App::DocumentObject*> SelectionSingleton::getObjectsOfType(const char* typeName, const char* pDocName, ResolveMode resolve) const
{
    Base::Type typeId = Base::Type::fromName(typeName);
    if (typeId == Base::Type::badType())
        return {};
    return getObjectsOfType(typeId, pDocName, resolve);
}

unsigned int SelectionSingleton::countObjectsOfType(const Base::Type& typeId, const char* pDocName, ResolveMode resolve) const
{
    unsigned int iNbr=0;
    App::Document *pcDoc = nullptr;
    if(!pDocName || strcmp(pDocName,"*") != 0) {
        pcDoc = getDocument(pDocName);
        if (!pcDoc)
            return 0;
    }

    for (auto &sel : _SelList) {
        if((!pcDoc||pcDoc==sel.pDoc) && getObjectOfType(sel, typeId, resolve))
            iNbr++;
    }

    return iNbr;
}

unsigned int SelectionSingleton::countObjectsOfType(const char* typeName, const char* pDocName, ResolveMode resolve) const
{
    Base::Type typeId = Base::Type::fromName(typeName);
    if (typeId == Base::Type::badType())
        return 0;
    return countObjectsOfType(typeId, pDocName, resolve);
}


void SelectionSingleton::slotSelectionChanged(const SelectionChanges& msg)
{
    if(msg.Type == SelectionChanges::SetPreselectSignal ||
       msg.Type == SelectionChanges::ShowSelection ||
       msg.Type == SelectionChanges::HideSelection)
        return;

    if(!msg.Object.getSubName().empty()) {
        auto pParent = msg.Object.getObject();
        if(!pParent)
            return;
        std::pair<std::string,std::string> elementName;
        auto &newElementName = elementName.first;
        auto &oldElementName = elementName.second;
        auto pObject = App::GeoFeature::resolveElement(pParent,msg.pSubName,elementName);
        if (!pObject)
            return;
        SelectionChanges msg2(msg.Type,pObject->getDocument()->getName(),
                pObject->getNameInDocument(),
                !newElementName.empty()?newElementName.c_str():oldElementName.c_str(),
                pObject->getTypeId().getName(), msg.x,msg.y,msg.z);

        try {
            msg2.pOriginalMsg = &msg;
            signalSelectionChanged3(msg2);

            msg2.Object.setSubName(oldElementName.c_str());
            msg2.pSubName = msg2.Object.getSubName().c_str();
            signalSelectionChanged2(msg2);
        }
        catch (const boost::exception&) {
            // reported by code analyzers
            Base::Console().Warning("slotSelectionChanged: Unexpected boost exception\n");
        }
    }
    else {
        try {
            signalSelectionChanged3(msg);
            signalSelectionChanged2(msg);
        }
        catch (const boost::exception&) {
            // reported by code analyzers
            Base::Console().Warning("slotSelectionChanged: Unexpected boost exception\n");
        }
    }
}

int SelectionSingleton::setPreselect(const char* pDocName, const char* pObjectName, const char* pSubName,
                                     float x, float y, float z, SelectionChanges::MsgSource signal)
{
    if (!pDocName || !pObjectName) {
        rmvPreselect();
        return 0;
    }
    if (!pSubName) pSubName = "";

    if (DocName==pDocName && FeatName==pObjectName && SubName==pSubName) {
        return -1;
    }

    rmvPreselect();

    if (ActiveGate && signal != SelectionChanges::MsgSource::Internal) {
        App::Document* pDoc = getDocument(pDocName);
        if (!pDoc || !pObjectName)
            return 0;
        std::pair<std::string,std::string> elementName;
        auto pObject = pDoc->getObject(pObjectName);
        if(!pObject)
            return 0;

        const char *subelement = pSubName;
        if (gateResolve != ResolveMode::NoResolve) {
            auto &newElementName = elementName.first;
            auto &oldElementName = elementName.second;
            pObject = App::GeoFeature::resolveElement(pObject,pSubName,elementName);
            if (!pObject)
                return 0;
            if (gateResolve > ResolveMode::OldStyleElement)
                subelement = !newElementName.empty() ? newElementName.c_str() : oldElementName.c_str();
            else
                subelement = oldElementName.c_str();
        }
        if (!ActiveGate->allow(pObject->getDocument(), pObject, subelement)) {
            QString msg;
            if (ActiveGate->notAllowedReason.length() > 0){
                msg = QObject::tr(ActiveGate->notAllowedReason.c_str());
            } else {
                msg = QCoreApplication::translate("SelectionFilter","Not allowed:");
            }
            msg.append(QString::fromLatin1(" %1.%2.%3 ")
                  .arg(QString::fromLatin1(pDocName),
                       QString::fromLatin1(pObjectName),
                       QString::fromLatin1(pSubName)));

            if (getMainWindow()) {
                getMainWindow()->showMessage(msg);
                Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                mdi->setOverrideCursor(QCursor(Qt::ForbiddenCursor));
            }
            return 0;
        }
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        mdi->restoreOverrideCursor();
    }

    DocName = pDocName;
    FeatName= pObjectName;
    SubName = pSubName;
    hx = x;
    hy = y;
    hz = z;

    // set up the change object
    SelectionChanges Chng(signal == SelectionChanges::MsgSource::Internal
                                  ? SelectionChanges::SetPreselectSignal
                                  : SelectionChanges::SetPreselect,
            DocName,FeatName,SubName,std::string(),x,y,z,signal);

    if (Chng.Type==SelectionChanges::SetPreselect) {
        CurrentPreselection = Chng;
        FC_TRACE("preselect "<<DocName<<'#'<<FeatName<<'.'<<SubName);
    }
    else {
        FC_TRACE("preselect signal "<<DocName<<'#'<<FeatName<<'.'<<SubName);
    }

    notify(Chng);

    if (signal == SelectionChanges::MsgSource::Internal && !DocName.empty()) {
        FC_TRACE("preselect "<<DocName<<'#'<<FeatName<<'.'<<SubName);
        Chng.Type = SelectionChanges::SetPreselect;
        CurrentPreselection = Chng;
        notify(std::move(Chng));
    }

    // It is possible the preselect is removed during notification
    return DocName.empty()?0:1;
}

namespace Gui {
std::array<std::pair<double, std::string>, 3> schemaTranslatePoint(double x, double y, double z, double precision)
{
    Base::Quantity mmx(Base::Quantity::MilliMetre);
    mmx.setValue(fabs(x) > precision ? x : 0.0);
    Base::Quantity mmy(Base::Quantity::MilliMetre);
    mmy.setValue(fabs(y) > precision ? y : 0.0);
    Base::Quantity mmz(Base::Quantity::MilliMetre);
    mmz.setValue(fabs(z) > precision ? z : 0.0);

    double xfactor, yfactor, zfactor;
    QString xunit, yunit, zunit;

    Base::UnitsApi::schemaTranslate(mmx, xfactor, xunit);
    Base::UnitsApi::schemaTranslate(mmy, yfactor, yunit);
    Base::UnitsApi::schemaTranslate(mmz, zfactor, zunit);

    double xuser = fabs(x) > precision ? x / xfactor : 0.0;
    double yuser = fabs(y) > precision ? y / yfactor : 0.0;
    double zuser = fabs(z) > precision ? z / zfactor : 0.0;

    std::array<std::pair<double, std::string>, 3> ret = {std::make_pair(xuser, xunit.toUtf8().constBegin()),
                                                         std::make_pair(yuser, yunit.toUtf8().constBegin()),
                                                         std::make_pair(zuser, zunit.toUtf8().constBegin())};
    return ret;
}
}

void SelectionSingleton::setPreselectCoord( float x, float y, float z)
{
    static char buf[513];

    // if nothing is in preselect ignore
    if(CurrentPreselection.Object.getObjectName().empty())
        return;

    CurrentPreselection.x = x;
    CurrentPreselection.y = y;
    CurrentPreselection.z = z;

    auto pts = schemaTranslatePoint(x, y, z, 0.0);
    snprintf(buf,512,"Preselected: %s.%s.%s (%f %s,%f %s,%f %s)"
                    ,CurrentPreselection.pDocName
                    ,CurrentPreselection.pObjectName
                    ,CurrentPreselection.pSubName
                    ,pts[0].first, pts[0].second.c_str()
                    ,pts[1].first, pts[1].second.c_str()
                    ,pts[2].first, pts[2].second.c_str());

    if (getMainWindow())
        getMainWindow()->showMessage(QString::fromUtf8(buf));
}

void SelectionSingleton::rmvPreselect(bool signal)
{
    if (DocName.empty())
        return;

    if(signal) {
        SelectionChanges Chng(SelectionChanges::RmvPreselectSignal,DocName,FeatName,SubName);
        notify(std::move(Chng));
        return;
    }

    SelectionChanges Chng(SelectionChanges::RmvPreselect,DocName,FeatName,SubName);

    // reset the current preselection
    CurrentPreselection = SelectionChanges();

    DocName = "";
    FeatName= "";
    SubName = "";
    hx = 0;
    hy = 0;
    hz = 0;

    if (ActiveGate && getMainWindow()) {
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        mdi->restoreOverrideCursor();
    }

    FC_TRACE("rmv preselect");

    // notify observing objects
    notify(std::move(Chng));
}

const SelectionChanges &SelectionSingleton::getPreselection() const
{
    return CurrentPreselection;
}

// add a SelectionGate to control what is selectable
void SelectionSingleton::addSelectionGate(Gui::SelectionGate *gate, ResolveMode resolve)
{
    if (ActiveGate)
        rmvSelectionGate();

    ActiveGate = gate;
    gateResolve = resolve;
}

// remove the active SelectionGate
void SelectionSingleton::rmvSelectionGate()
{
    if (ActiveGate) {
        delete ActiveGate;
        ActiveGate = nullptr;

        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            // if a document is about to be closed it has no MDI view any more
            Gui::MDIView* mdi = doc->getActiveView();
            if (mdi)
                mdi->restoreOverrideCursor();
        }
    }
}


App::Document* SelectionSingleton::getDocument(const char* pDocName) const
{
    if (pDocName && pDocName[0])
        return App::GetApplication().getDocument(pDocName);
    else
        return App::GetApplication().getActiveDocument();
}

int SelectionSingleton::disableCommandLog() {
    if(!logDisabled)
        logHasSelection = hasSelection();
    return ++logDisabled;
}

int SelectionSingleton::enableCommandLog(bool silent) {
    --logDisabled;
    if(!logDisabled && !silent) {
        auto manager = Application::Instance->macroManager();
        if(!hasSelection()) {
            if(logHasSelection)
                manager->addLine(MacroManager::Cmt, "Gui.Selection.clearSelection()");
        }else{
            for(auto &sel : _SelList)
                sel.log();
        }
    }
    return logDisabled;
}

void SelectionSingleton::_SelObj::log(bool remove, bool clearPreselect) {
    if(logged && !remove)
        return;
    logged = true;
    std::ostringstream ss;
    ss << "Gui.Selection." << (remove?"removeSelection":"addSelection")
        << "('" << DocName  << "','" << FeatName << "'";
    if(!SubName.empty()) {
        if(!elementName.second.empty() && !elementName.first.empty())
            ss << ",'" << SubName.substr(0,SubName.size()-elementName.first.size())
                << elementName.second << "'";
        else
            ss << ",'" << SubName << "'";
    }
    if(!remove && (x || y || z || !clearPreselect)) {
        if(SubName.empty())
            ss << ",''";
        ss << ',' << x << ',' << y << ',' << z;
        if(!clearPreselect)
            ss << ",False";
    }
    ss << ')';
    Application::Instance->macroManager()->addLine(MacroManager::Cmt, ss.str().c_str());
}

bool SelectionSingleton::addSelection(const char* pDocName, const char* pObjectName,
        const char* pSubName, float x, float y, float z,
        const std::vector<SelObj> *pickedList, bool clearPreselect)
{
    if(pickedList) {
        _PickedList.clear();
        for(const auto &sel : *pickedList) {
            _PickedList.emplace_back();
            auto &s = _PickedList.back();
            s.DocName = sel.DocName;
            s.FeatName = sel.FeatName;
            s.SubName = sel.SubName;
            s.TypeName = sel.TypeName;
            s.pObject = sel.pObject;
            s.pDoc = sel.pDoc;
            s.x = sel.x;
            s.y = sel.y;
            s.z = sel.z;
        }
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    _SelObj temp;
    int ret = checkSelection(pDocName, pObjectName, pSubName, ResolveMode::NoResolve, temp);
    if (ret!=0)
        return false;

    temp.x        = x;
    temp.y        = y;
    temp.z        = z;

    // check for a Selection Gate
    if (ActiveGate) {
        const char *subelement = nullptr;
        auto pObject = getObjectOfType(temp,App::DocumentObject::getClassTypeId(),gateResolve,&subelement);
        if (!ActiveGate->allow(pObject?pObject->getDocument():temp.pDoc,pObject,subelement)) {
            if (getMainWindow()) {
                QString msg;
                if (ActiveGate->notAllowedReason.length() > 0) {
                    msg = QObject::tr(ActiveGate->notAllowedReason.c_str());
                } else {
                    msg = QCoreApplication::translate("SelectionFilter","Selection not allowed by filter");
                }
                getMainWindow()->showMessage(msg);
                Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                mdi->setOverrideCursor(Qt::ForbiddenCursor);
            }
            ActiveGate->notAllowedReason.clear();
            QApplication::beep();
            return false;
        }
    }

    if(!logDisabled)
        temp.log(false,clearPreselect);

    _SelList.push_back(temp);
    _SelStackForward.clear();

    if(clearPreselect)
        rmvPreselect();

    SelectionChanges Chng(SelectionChanges::AddSelection,
            temp.DocName,temp.FeatName,temp.SubName,temp.TypeName, x,y,z);

    FC_LOG("Add Selection "<<Chng.pDocName<<'#'<<Chng.pObjectName<<'.'<<Chng.pSubName
            << " (" << x << ", " << y << ", " << z << ')');

    notify(std::move(Chng));

    getMainWindow()->updateActions();

    rmvPreselect(true);

    // There is a possibility that some observer removes or clears selection
    // inside signal handler, hence the check here
    return isSelected(temp.DocName.c_str(),temp.FeatName.c_str(), temp.SubName.c_str());
}

void SelectionSingleton::selStackPush(bool clearForward, bool overwrite) {
    static int stackSize;
    if(!stackSize) {
        stackSize = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/View")->GetInt("SelectionStackSize",100);
    }
    if(clearForward)
        _SelStackForward.clear();
    if(_SelList.empty())
        return;
    if((int)_SelStackBack.size() >= stackSize)
        _SelStackBack.pop_front();
    SelStackItem item;
    for(auto &sel : _SelList)
        item.emplace(sel.DocName.c_str(),sel.FeatName.c_str(),sel.SubName.c_str());
    if(!_SelStackBack.empty() && _SelStackBack.back()==item)
        return;
    if(!overwrite || _SelStackBack.empty())
        _SelStackBack.emplace_back();
    _SelStackBack.back() = std::move(item);
}

void SelectionSingleton::selStackGoBack(int count) {
    if((int)_SelStackBack.size()<count)
        count = _SelStackBack.size();
    if(count<=0)
        return;
    if(!_SelList.empty()) {
        selStackPush(false,true);
        clearCompleteSelection();
    } else
        --count;
    for(int i=0;i<count;++i) {
        _SelStackForward.push_front(std::move(_SelStackBack.back()));
        _SelStackBack.pop_back();
    }
    std::deque<SelStackItem> tmpStack;
    _SelStackForward.swap(tmpStack);
    while(!_SelStackBack.empty()) {
        bool found = false;
        for(auto &sobjT : _SelStackBack.back()) {
            if(sobjT.getSubObject()) {
                addSelection(sobjT.getDocumentName().c_str(),
                             sobjT.getObjectName().c_str(),
                             sobjT.getSubName().c_str());
                found = true;
            }
        }
        if(found)
            break;
        tmpStack.push_front(std::move(_SelStackBack.back()));
        _SelStackBack.pop_back();
    }
    _SelStackForward = std::move(tmpStack);
    getMainWindow()->updateActions();
}

void SelectionSingleton::selStackGoForward(int count) {
    if((int)_SelStackForward.size()<count)
        count = _SelStackForward.size();
    if(count<=0)
        return;
    if(!_SelList.empty()) {
        selStackPush(false,true);
        clearCompleteSelection();
    }
    for(int i=0;i<count;++i) {
        _SelStackBack.push_back(_SelStackForward.front());
        _SelStackForward.pop_front();
    }
    std::deque<SelStackItem> tmpStack;
    _SelStackForward.swap(tmpStack);
    while(true) {
        bool found = false;
        for(auto &sobjT : _SelStackBack.back()) {
            if(sobjT.getSubObject()) {
                addSelection(sobjT.getDocumentName().c_str(),
                             sobjT.getObjectName().c_str(),
                             sobjT.getSubName().c_str());
                found = true;
            }
        }
        if(found || tmpStack.empty())
            break;
        _SelStackBack.push_back(tmpStack.front());
        tmpStack.pop_front();
    }
    _SelStackForward = std::move(tmpStack);
    getMainWindow()->updateActions();
}

std::vector<SelectionObject> SelectionSingleton::selStackGet(const char* pDocName, ResolveMode resolve, int index) const
{
    const SelStackItem *item = nullptr;
    if (index >= 0) {
        if(index >= (int)_SelStackBack.size())
            return {};
        item = &_SelStackBack[_SelStackBack.size()-1-index];
    }
    else {
        index = -index-1;
        if(index>=(int)_SelStackForward.size())
            return {};
        item = &_SelStackBack[_SelStackForward.size()-1-index];
    }

    std::list<_SelObj> selList;
    for(auto &sobjT : *item) {
        _SelObj sel;
        if(checkSelection(sobjT.getDocumentName().c_str(),
                          sobjT.getObjectName().c_str(),
                          sobjT.getSubName().c_str(),
                          ResolveMode::NoResolve,
                          sel,
                          &selList)==0)
        {
            selList.push_back(sel);
        }
    }

    return getObjectList(pDocName,App::DocumentObject::getClassTypeId(), selList, resolve);
}

bool SelectionSingleton::addSelections(const char* pDocName, const char* pObjectName, const std::vector<std::string>& pSubNames)
{
    if(!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    bool update = false;
    for(const auto & pSubName : pSubNames) {
        _SelObj temp;
        int ret = checkSelection(pDocName, pObjectName, pSubName.c_str(), ResolveMode::NoResolve, temp);
        if (ret!=0)
            continue;

        temp.x        = 0;
        temp.y        = 0;
        temp.z        = 0;

        _SelList.push_back(temp);
        _SelStackForward.clear();

        SelectionChanges Chng(SelectionChanges::AddSelection,
                temp.DocName,temp.FeatName,temp.SubName,temp.TypeName);

        FC_LOG("Add Selection "<<Chng.pDocName<<'#'<<Chng.pObjectName<<'.'<<Chng.pSubName);

        notify(std::move(Chng));
        update = true;
    }

    if(update)
        getMainWindow()->updateActions();
    return true;
}

bool SelectionSingleton::updateSelection(bool show, const char* pDocName,
                            const char* pObjectName, const char* pSubName)
{
    if(!pDocName || !pObjectName)
        return false;
    if(!pSubName)
        pSubName = "";
    if(DocName==pDocName && FeatName==pObjectName && SubName==pSubName) {
        if(show) {
            FC_TRACE("preselect signal");
            notify(SelectionChanges(SelectionChanges::SetPreselectSignal,DocName,FeatName,SubName));
        }else
            rmvPreselect();
    }
    auto pDoc = getDocument(pDocName);
    if(!pDoc)
        return false;
    auto pObject = pDoc->getObject(pObjectName);
    if(!pObject)
        return false;
    if (!isSelected(pObject, pSubName, ResolveMode::NoResolve))
        return false;

    SelectionChanges Chng(show?SelectionChanges::ShowSelection:SelectionChanges::HideSelection,
            pDocName,pObjectName,pSubName,pObject->getTypeId().getName());

    FC_LOG("Update Selection "<<Chng.pDocName << '#' << Chng.pObjectName << '.' <<Chng.pSubName);

    notify(std::move(Chng));

    return true;
}

bool SelectionSingleton::addSelection(const SelectionObject& obj, bool clearPreselect)
{
    const std::vector<std::string>& subNames = obj.getSubNames();
    const std::vector<Base::Vector3d> points = obj.getPickedPoints();
    if (!subNames.empty() && subNames.size() == points.size()) {
        bool ok = true;
        for (std::size_t i=0; i<subNames.size(); i++) {
            const std::string& name = subNames[i];
            const Base::Vector3d& pnt = points[i];
            ok &= addSelection(obj.getDocName(), obj.getFeatName(), name.c_str(),
                               static_cast<float>(pnt.x),
                               static_cast<float>(pnt.y),
                               static_cast<float>(pnt.z),nullptr,clearPreselect);
        }
        return ok;
    }
    else if (!subNames.empty()) {
        bool ok = true;
        for (const std::string& name : subNames) {
            ok &= addSelection(obj.getDocName(), obj.getFeatName(), name.c_str());
        }
        return ok;
    }
    else {
        return addSelection(obj.getDocName(), obj.getFeatName());
    }
}


void SelectionSingleton::rmvSelection(const char* pDocName, const char* pObjectName, const char* pSubName,
        const std::vector<SelObj> *pickedList)
{
    if(pickedList) {
        _PickedList.clear();
        for(const auto &sel : *pickedList) {
            _PickedList.emplace_back();
            auto &s = _PickedList.back();
            s.DocName = sel.DocName;
            s.FeatName = sel.FeatName;
            s.SubName = sel.SubName;
            s.TypeName = sel.TypeName;
            s.pObject = sel.pObject;
            s.pDoc = sel.pDoc;
            s.x = sel.x;
            s.y = sel.y;
            s.z = sel.z;
        }
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    if(!pDocName)
        return;

    _SelObj temp;
    int ret = checkSelection(pDocName, pObjectName, pSubName, ResolveMode::NoResolve, temp);
    if (ret<0)
        return;

    std::vector<SelectionChanges> changes;
    for(auto It=_SelList.begin(),ItNext=It;It!=_SelList.end();It=ItNext) {
        ++ItNext;
        if(It->DocName!=temp.DocName || It->FeatName!=temp.FeatName)
            continue;
        // if no subname is specified, remove all subobjects of the matching object
        if(!temp.SubName.empty()) {
            // otherwise, match subojects with common prefix, separated by '.'
            if(!boost::starts_with(It->SubName,temp.SubName) ||
               (It->SubName.length()!=temp.SubName.length() && It->SubName[temp.SubName.length()-1]!='.'))
                continue;
        }

        It->log(true);

        changes.emplace_back(SelectionChanges::RmvSelection,
                It->DocName,It->FeatName,It->SubName,It->TypeName);

        // destroy the _SelObj item
        _SelList.erase(It);
    }

    // NOTE: It can happen that there are nested calls of rmvSelection()
    // so that it's not safe to invoke the notifications inside the loop
    // as this can invalidate the iterators and thus leads to undefined
    // behaviour.
    // So, the notification is done after the loop, see also #0003469
    if(!changes.empty()) {
        for(auto &Chng : changes) {
            FC_LOG("Rmv Selection "<<Chng.pDocName<<'#'<<Chng.pObjectName<<'.'<<Chng.pSubName);
            notify(std::move(Chng));
        }
        getMainWindow()->updateActions();
    }
}

struct SelInfo {
    std::string DocName;
    std::string FeatName;
    std::string SubName;
    SelInfo(const std::string &docName,
            const std::string &featName,
            const std::string &subName)
        :DocName(docName)
        ,FeatName(featName)
        ,SubName(subName)
    {}
};

void SelectionSingleton::setVisible(VisibleState vis) {
    std::set<std::pair<App::DocumentObject*,App::DocumentObject*> > filter;
    int visible;
    switch(vis) {
    case VisShow:
        visible = 1;
        break;
    case VisToggle:
        visible = -1;
        break;
    default:
        visible = 0;
    }

    // Copy the selection in case it changes during this function
    std::vector<SelInfo> sels;
    sels.reserve(_SelList.size());
    for(auto &sel : _SelList) {
        if(sel.DocName.empty() || sel.FeatName.empty() || !sel.pObject)
            continue;
        sels.emplace_back(sel.DocName,sel.FeatName,sel.SubName);
    }

    for(auto &sel : sels) {
        App::Document *doc = App::GetApplication().getDocument(sel.DocName.c_str());
        if(!doc) continue;
        App::DocumentObject *obj = doc->getObject(sel.FeatName.c_str());
        if(!obj) continue;

        // get parent object
        App::DocumentObject *parent = nullptr;
        std::string elementName;
        obj = obj->resolve(sel.SubName.c_str(),&parent,&elementName);
        if (!obj || !obj->getNameInDocument() || (parent && !parent->getNameInDocument()))
            continue;
        // try call parent object's setElementVisible
        if (parent) {
            // prevent setting the same object visibility more than once
            if (!filter.insert(std::make_pair(obj,parent)).second)
                continue;
            int visElement = parent->isElementVisible(elementName.c_str());
            if (visElement >= 0) {
                if (visElement > 0)
                    visElement = 1;
                if (visible >= 0) {
                    if (visElement == visible)
                        continue;
                    visElement = visible;
                }
                else {
                    visElement = !visElement;
                }

                if (!visElement)
                    updateSelection(false,sel.DocName.c_str(),sel.FeatName.c_str(), sel.SubName.c_str());
                parent->setElementVisible(elementName.c_str(), visElement ? true : false);
                if (visElement)
                    updateSelection(true,sel.DocName.c_str(),sel.FeatName.c_str(), sel.SubName.c_str());
                continue;
            }

            // Fall back to direct object visibility setting
        }
        if(!filter.insert(std::make_pair(obj,static_cast<App::DocumentObject*>(nullptr))).second){
            continue;
        }

        auto vp = Application::Instance->getViewProvider(obj);

        if(vp) {
            bool visObject;
            if(visible>=0)
                visObject = visible ? true : false;
            else
                visObject = !vp->isShow();

            if(visObject) {
                vp->show();
                updateSelection(visObject,sel.DocName.c_str(),sel.FeatName.c_str(), sel.SubName.c_str());
            } else {
                updateSelection(visObject,sel.DocName.c_str(),sel.FeatName.c_str(), sel.SubName.c_str());
                vp->hide();
            }
        }
    }
}

void SelectionSingleton::setSelection(const char* pDocName, const std::vector<App::DocumentObject*>& sel)
{
    App::Document *pcDoc;
    pcDoc = getDocument(pDocName);
    if (!pcDoc)
        return;

    if(!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    bool touched = false;
    for(auto obj : sel) {
        if(!obj || !obj->getNameInDocument())
            continue;
        _SelObj temp;
        int ret = checkSelection(pDocName,obj->getNameInDocument(), nullptr, ResolveMode::NoResolve, temp);
        if (ret!=0)
            continue;
        touched = true;
        _SelList.push_back(temp);
    }

    if(touched) {
        _SelStackForward.clear();
        notify(SelectionChanges(SelectionChanges::SetSelection, pDocName));
        getMainWindow()->updateActions();
    }
}

void SelectionSingleton::clearSelection(const char* pDocName, bool clearPreSelect)
{
    // Because the introduction of external editing, it is best to make
    // clearSelection(0) behave as clearCompleteSelection(), which is the same
    // behavior of python Selection.clearSelection(None)
    if (!pDocName || !pDocName[0] || strcmp(pDocName,"*")==0) {
        clearCompleteSelection(clearPreSelect);
        return;
    }

    if (!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    App::Document* pDoc;
    pDoc = getDocument(pDocName);
    if (pDoc) {
        std::string docName = pDocName;
        if (clearPreSelect && DocName == docName)
            rmvPreselect();

        bool touched = false;
        for (auto it=_SelList.begin();it!=_SelList.end();) {
            if (it->DocName == docName) {
                touched = true;
                it = _SelList.erase(it);
            }
            else {
                ++it;
            }
        }

        if (!touched)
            return;

        if (!logDisabled) {
            std::ostringstream ss;
            ss << "Gui.Selection.clearSelection('" << docName << "'";
            if (!clearPreSelect)
                ss << ", False";
            ss << ')';
            Application::Instance->macroManager()->addLine(MacroManager::Cmt,ss.str().c_str());
        }

        notify(SelectionChanges(SelectionChanges::ClrSelection,docName.c_str()));

        getMainWindow()->updateActions();
    }
}

void SelectionSingleton::clearCompleteSelection(bool clearPreSelect)
{
    if(!_PickedList.empty()) {
        _PickedList.clear();
        notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }

    if(clearPreSelect)
        rmvPreselect();

    if(_SelList.empty())
        return;

    if(!logDisabled)
        Application::Instance->macroManager()->addLine(MacroManager::Cmt,
                clearPreSelect?"Gui.Selection.clearSelection()"
                              :"Gui.Selection.clearSelection(False)");

    _SelList.clear();

    SelectionChanges Chng(SelectionChanges::ClrSelection);

    FC_LOG("Clear selection");

    notify(std::move(Chng));
    getMainWindow()->updateActions();
}

bool SelectionSingleton::isSelected(const char* pDocName, const char* pObjectName,
                                    const char* pSubName, ResolveMode resolve) const
{
    _SelObj sel;
    return checkSelection(pDocName, pObjectName, pSubName, resolve, sel, &_SelList) > 0;
}

bool SelectionSingleton::isSelected(App::DocumentObject* pObject, const char* pSubName, ResolveMode resolve) const
{
    if (!pObject || !pObject->getNameInDocument() || !pObject->getDocument())
        return false;
    _SelObj sel;
    return checkSelection(pObject->getDocument()->getName(),
            pObject->getNameInDocument(), pSubName, resolve, sel, &_SelList) > 0;
}

int SelectionSingleton::checkSelection(const char *pDocName, const char *pObjectName, const char *pSubName,
                                       ResolveMode resolve, _SelObj &sel, const std::list<_SelObj> *selList) const
{
    sel.pDoc = getDocument(pDocName);
    if(!sel.pDoc) {
        if(!selList)
            FC_ERR("Cannot find document");
        return -1;
    }
    pDocName = sel.pDoc->getName();
    sel.DocName = pDocName;

    if(pObjectName)
        sel.pObject = sel.pDoc->getObject(pObjectName);
    else
        sel.pObject = nullptr;
    if (!sel.pObject) {
        if(!selList)
            FC_ERR("Object not found");
        return -1;
    }
    if (sel.pObject->testStatus(App::ObjectStatus::Remove))
        return -1;
    if (pSubName)
       sel.SubName = pSubName;
    if (resolve == ResolveMode::NoResolve)
        TreeWidget::checkTopParent(sel.pObject,sel.SubName);
    pSubName = !sel.SubName.empty()?sel.SubName.c_str():nullptr;
    sel.FeatName = sel.pObject->getNameInDocument();
    sel.TypeName = sel.pObject->getTypeId().getName();
    const char *element = nullptr;
    sel.pResolvedObject = App::GeoFeature::resolveElement(sel.pObject,
            pSubName,sel.elementName,false,App::GeoFeature::Normal,nullptr,&element);
    if(!sel.pResolvedObject) {
        if(!selList)
            FC_ERR("Sub-object " << sel.DocName << '#' << sel.FeatName << '.' << sel.SubName << " not found");
        return -1;
    }
    if(sel.pResolvedObject->testStatus(App::ObjectStatus::Remove))
        return -1;
    std::string subname;
    std::string prefix;
    if(pSubName && element) {
        prefix = std::string(pSubName, element-pSubName);
        if(!sel.elementName.first.empty()) {
            // make sure the selected sub name is a new style if available
            subname = prefix + sel.elementName.first;
            pSubName = subname.c_str();
            sel.SubName = subname;
        }
    }
    if(!selList)
        selList = &_SelList;

    if(!pSubName)
        pSubName = "";

    for (auto &s : *selList) {
        if (s.DocName==pDocName && s.FeatName==sel.FeatName) {
            if(s.SubName==pSubName)
                return 1;
            if (resolve > ResolveMode::OldStyleElement && boost::starts_with(s.SubName,prefix))
                return 1;
        }
    }
    if (resolve == ResolveMode::OldStyleElement) {
        for(auto &s : *selList) {
            if(s.pResolvedObject != sel.pResolvedObject)
                continue;
            if(!pSubName[0])
                return 1;
            if (!s.elementName.first.empty()) {
                if (s.elementName.first == sel.elementName.first)
                    return 1;
            }
            else if(s.SubName == sel.elementName.second)
                return 1;
        }
    }
    return 0;
}

const char *SelectionSingleton::getSelectedElement(App::DocumentObject *obj, const char* pSubName) const
{
    if (!obj)
        return nullptr;

    for(list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        if (It->pObject == obj) {
            auto len = It->SubName.length();
            if(!len)
                return "";
            if (pSubName && strncmp(pSubName,It->SubName.c_str(),It->SubName.length())==0){
                if(pSubName[len]==0 || pSubName[len-1] == '.')
                    return It->SubName.c_str();
            }
        }
    }
    return nullptr;
}

void SelectionSingleton::slotDeletedObject(const App::DocumentObject& Obj)
{
    if(!Obj.getNameInDocument())
        return;

    // For safety reason, don't bother checking
    rmvPreselect();

    // Remove also from the selection, if selected
    // We don't walk down the hierarchy for each selection, so there may be stray selection
    std::vector<SelectionChanges> changes;
    for(auto it=_SelList.begin(),itNext=it;it!=_SelList.end();it=itNext) {
        ++itNext;
        if(it->pResolvedObject == &Obj || it->pObject==&Obj) {
            changes.emplace_back(SelectionChanges::RmvSelection,
                    it->DocName,it->FeatName,it->SubName,it->TypeName);
            _SelList.erase(it);
        }
    }
    if(!changes.empty()) {
        for(auto &Chng : changes) {
            FC_LOG("Rmv Selection "<<Chng.pDocName<<'#'<<Chng.pObjectName<<'.'<<Chng.pSubName);
            notify(std::move(Chng));
        }
        getMainWindow()->updateActions();
    }

    if(!_PickedList.empty()) {
        bool changed = false;
        for(auto it=_PickedList.begin(),itNext=it;it!=_PickedList.end();it=itNext) {
            ++itNext;
            auto &sel = *it;
            if(sel.DocName == Obj.getDocument()->getName() &&
               sel.FeatName == Obj.getNameInDocument())
            {
                changed = true;
                _PickedList.erase(it);
            }
        }
        if(changed)
            notify(SelectionChanges(SelectionChanges::PickedListChanged));
    }
}

void SelectionSingleton::setSelectionStyle(SelectionStyle selStyle)
{
    selectionStyle = selStyle;
}

SelectionSingleton::SelectionStyle SelectionSingleton::getSelectionStyle()
{
    return selectionStyle;
}

//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
SelectionSingleton::SelectionSingleton() :
    CurrentPreselection(SelectionChanges::ClrSelection),
    selectionStyle(SelectionStyle::NormalSelection)
{
    hx = 0;
    hy = 0;
    hz = 0;
    ActiveGate = nullptr;
    gateResolve = ResolveMode::OldStyleElement;
    //NOLINTBEGIN
    App::GetApplication().signalDeletedObject.connect(std::bind(&Gui::SelectionSingleton::slotDeletedObject, this, sp::_1));
    signalSelectionChanged.connect(std::bind(&Gui::SelectionSingleton::slotSelectionChanged, this, sp::_1));
    //NOLINTEND
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
SelectionSingleton::~SelectionSingleton() = default;

SelectionSingleton* SelectionSingleton::_pcSingleton = nullptr;

SelectionSingleton& SelectionSingleton::instance()
{
    if (!_pcSingleton)
        _pcSingleton = new SelectionSingleton;
    return *_pcSingleton;
}

void SelectionSingleton::destruct ()
{
    if (_pcSingleton)
        delete _pcSingleton;
    _pcSingleton = nullptr;
}

//**************************************************************************
// Python stuff

// SelectionSingleton Methods  // Methods structure
PyMethodDef SelectionSingleton::Methods[] = {
    {"addSelection",         (PyCFunction) SelectionSingleton::sAddSelection, METH_VARARGS,
     "addSelection(docName, objName, subName, x=0, y=0, z=0, clear=True) -> None\n"
     "addSelection(obj, subName, x=0, y=0, z=0, clear=True) -> None\n"
     "addSelection(obj, subNames, clear=True) -> None\n"
     "\n"
     "Add an object to the selection.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "objName : str\n    Name of the `App.DocumentObject` to add.\n"
     "obj : App.DocumentObject\n    Object to add.\n"
     "subName : str\n    Subelement name.\n"
     "x : float\n    Coordinate `x` of the point to pick.\n"
     "y : float\n    Coordinate `y` of the point to pick.\n"
     "z : float\n    Coordinate `z` of the point to pick.\n"
     "subNames : list of str\n    List of subelement names.\n"
     "clear : bool\n    Clear preselection."},
    {"updateSelection",      (PyCFunction) SelectionSingleton::sUpdateSelection, METH_VARARGS,
     "updateSelection(show, obj, subName) -> None\n"
     "\n"
     "Update an object in the selection.\n"
     "\n"
     "show : bool\n    Show or hide the selection.\n"
     "obj : App.DocumentObject\n    Object to update.\n"
     "subName : str\n    Name of the subelement to update."},
    {"removeSelection",      (PyCFunction) SelectionSingleton::sRemoveSelection, METH_VARARGS,
     "removeSelection(obj, subName) -> None\n"
     "removeSelection(docName, objName, subName) -> None\n"
     "\n"
     "Remove an object from the selection.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "objName : str\n    Name of the `App.DocumentObject` to remove.\n"
     "obj : App.DocumentObject\n    Object to remove.\n"
     "subName : str\n    Name of the subelement to remove."},
    {"clearSelection"  ,     (PyCFunction) SelectionSingleton::sClearSelection, METH_VARARGS,
     "clearSelection(docName, clearPreSelect=True) -> None\n"
     "clearSelection(clearPreSelect=True) -> None\n"
     "\n"
     "Clear the selection in the given document. If no document is\n"
     "given the complete selection is cleared.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "clearPreSelect : bool\n    Clear preselection."},
    {"isSelected",           (PyCFunction) SelectionSingleton::sIsSelected, METH_VARARGS,
     "isSelected(obj, subName, resolve=ResolveMode.OldStyleElement) -> bool\n"
     "\n"
     "Check if a given object is selected.\n"
     "\n"
     "obj : App.DocumentObject\n    Object to check.\n"
     "subName : str\n    Name of the subelement.\n"
     "resolve : int\n    Resolve subelement reference."},
    {"setPreselection",      reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) ()>( SelectionSingleton::sSetPreselection )), METH_VARARGS|METH_KEYWORDS,
     "setPreselection(obj, subName, x=0, y=0, z=0, type=1) -> None\n"
     "\n"
     "Set preselected object.\n"
     "\n"
     "obj : App.DocumentObject\n    Object to preselect.\n"
     "subName : str\n    Subelement name.\n"
     "x : float\n    Coordinate `x` of the point to preselect.\n"
     "y : float\n    Coordinate `y` of the point to preselect.\n"
     "z : float\n    Coordinate `z` of the point to preselect.\n"
     "type : int"},
    {"getPreselection",      (PyCFunction) SelectionSingleton::sGetPreselection, METH_VARARGS,
    "getPreselection() -> Gui.SelectionObject\n"
    "\n"
    "Get preselected object."},
    {"clearPreselection",   (PyCFunction) SelectionSingleton::sRemPreselection, METH_VARARGS,
     "clearPreselection() -> None\n"
     "\n"
     "Clear the preselection."},
    {"countObjectsOfType",   (PyCFunction) SelectionSingleton::sCountObjectsOfType, METH_VARARGS,
     "countObjectsOfType(type, docName, resolve=ResolveMode.OldStyleElement) -> int\n"
     "\n"
     "Get the number of selected objects. If no document name is given the\n"
     "active document is used and '*' means all documents.\n"
     "\n"
     "type : str\n    Object type id name.\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int"},
    {"getSelection",         (PyCFunction) SelectionSingleton::sGetSelection, METH_VARARGS,
     "getSelection(docName, resolve=ResolveMode.OldStyleElement, single=False) -> list\n"
     "\n"
     "Return a list of selected objects. If no document name is given\n"
     "the active document is used and '*' means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int\n    Resolve the subname references.\n"
     "    0: do not resolve, 1: resolve, 2: resolve with element map.\n"
     "single : bool\n    Only return if there is only one selection."},
    {"getPickedList",         (PyCFunction) SelectionSingleton::sGetPickedList, 1,
     "getPickedList(docName) -> list of Gui.SelectionObject\n"
     "\n"
     "Return a list of SelectionObjects generated by the last mouse click.\n"
     "If no document name is given the active document is used and '*'\n"
     "means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`."},
    {"enablePickedList",      (PyCFunction) SelectionSingleton::sEnablePickedList, METH_VARARGS,
     "enablePickedList(enable=True) -> None\n"
     "\n"
     "Enable/disable pick list.\n"
     "\n"
     "enable : bool"},
    {"getCompleteSelection", (PyCFunction) SelectionSingleton::sGetCompleteSelection, METH_VARARGS,
     "getCompleteSelection(resolve=ResolveMode.OldStyleElement) -> list\n"
     "\n"
     "Return a list of selected objects across all documents.\n"
     "\n"
     "resolve : int"},
    {"getSelectionEx",         (PyCFunction) SelectionSingleton::sGetSelectionEx, METH_VARARGS,
     "getSelectionEx(docName, resolve=ResolveMode.OldStyleElement, single=False) -> list of Gui.SelectionObject\n"
     "\n"
     "Return a list of SelectionObjects. If no document name is given the\n"
     "active document is used and '*' means all documents.\n"
     "The SelectionObjects contain a variety of information about the selection,\n"
     "e.g. subelement names.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int\n    Resolve the subname references.\n"
     "    0: do not resolve, 1: resolve, 2: resolve with element map.\n"
     "single : bool\n    Only return if there is only one selection."},
    {"getSelectionObject",  (PyCFunction) SelectionSingleton::sGetSelectionObject, METH_VARARGS,
     "getSelectionObject(docName, objName, subName, point) -> Gui.SelectionObject\n"
     "\n"
     "Return a SelectionObject.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "objName : str\n    Name of the `App.DocumentObject` to select.\n"
     "subName : str\n    Subelement name.\n"
     "point : tuple\n    Coordinates of the point to pick."},
    {"setSelectionStyle",         (PyCFunction) SelectionSingleton::sSetSelectionStyle, METH_VARARGS,
     "setSelectionStyle(selectionStyle) -> None\n"
     "\n"
     "Change the selection style. 0 for normal selection, 1 for greedy selection\n"
     "\n"
     "selectionStyle : int"},
    {"addObserver",         (PyCFunction) SelectionSingleton::sAddSelObserver, METH_VARARGS,
     "addObserver(object, resolve=ResolveMode.OldStyleElement) -> None\n"
     "\n"
     "Install an observer.\n"
     "\n"
     "object : object\n    Python object instance.\n"
     "resolve : int"},
    {"removeObserver",      (PyCFunction) SelectionSingleton::sRemSelObserver, METH_VARARGS,
     "removeObserver(object) -> None\n"
     "\n"
     "Uninstall an observer.\n"
     "\n"
     "object : object\n    Python object instance."},
    {"addSelectionGate",      (PyCFunction) SelectionSingleton::sAddSelectionGate, METH_VARARGS,
     "addSelectionGate(filter, resolve=ResolveMode.OldStyleElement) -> None\n"
     "\n"
     "Activate the selection gate.\n"
     "The selection gate will prohibit all selections that do not match\n"
     "the given selection criteria.\n"
     "\n"
     "filter : str, SelectionFilter, object\n"
     "resolve : int\n"
     "\n"
     "Examples strings are:\n"
     "Gui.Selection.addSelectionGate('SELECT Part::Feature SUBELEMENT Edge')\n"
     "Gui.Selection.addSelectionGate('SELECT Robot::RobotObject')\n"
     "\n"
     "An instance of SelectionFilter can also be set:\n"
     "filter = Gui.Selection.Filter('SELECT Part::Feature SUBELEMENT Edge')\n"
     "Gui.Selection.addSelectionGate(filter)\n"
     "\n"
     "The most flexible approach is to write a selection gate class that\n"
     "implements the method 'allow':\n"
     "class Gate:\n"
     "    def allow(self,doc,obj,sub):\n"
     "        return (sub[0:4] == 'Face')\n"
     "Gui.Selection.addSelectionGate(Gate())"},
    {"removeSelectionGate",      (PyCFunction) SelectionSingleton::sRemoveSelectionGate, METH_VARARGS,
     "removeSelectionGate() -> None\n"
     "\n"
     "Remove the active selection gate."},
    {"setVisible",            (PyCFunction) SelectionSingleton::sSetVisible, METH_VARARGS,
     "setVisible(visible=None) -> None\n"
     "\n"
     "Set visibility of all selection items.\n"
     "\n"
     "visible : bool, None\n    If None, then toggle visibility."},
    {"pushSelStack",      (PyCFunction) SelectionSingleton::sPushSelStack, METH_VARARGS,
     "pushSelStack(clearForward=True, overwrite=False) -> None\n"
     "\n"
     "Push current selection to stack.\n"
     "\n"
     "clearForward : bool\n    Clear the forward selection stack.\n"
     "overwrite : bool\n    Overwrite the top back selection stack with current selection."},
    {"hasSelection",      (PyCFunction) SelectionSingleton::sHasSelection, METH_VARARGS,
     "hasSelection(docName, resolve=ResolveMode.NoResolve) -> bool\n"
     "\n"
     "Check if there is any selection. If no document name is given,\n"
     "checks selections in all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int"},
    {"hasSubSelection",   (PyCFunction) SelectionSingleton::sHasSubSelection, METH_VARARGS,
     "hasSubSelection(docName, subElement=False) -> bool\n"
     "\n"
     "Check if there is any selection with subname. If no document name\n"
     "is given the active document is used and '*' means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "subElement : bool"},
    {"getSelectionFromStack",(PyCFunction) SelectionSingleton::sGetSelectionFromStack, METH_VARARGS,
     "getSelectionFromStack(docName, resolve=ResolveMode.OldStyleElement, index=0) -> list of Gui.SelectionObject\n"
     "\n"
     "Return SelectionObjects from selection stack. If no document name is given\n"
     "the active document is used and '*' means all documents.\n"
     "\n"
     "docName : str\n    Name of the `App.Document`.\n"
     "resolve : int\n    Resolve the subname references.\n"
     "    0: do not resolve, 1: resolve, 2: resolve with element map.\n"
     "index : int\n    Select stack index.\n"
     "    0: last pushed selection, > 0: trace back, < 0: trace forward."},
    {nullptr, nullptr, 0, nullptr}  /* Sentinel */
};

PyObject *SelectionSingleton::sAddSelection(PyObject * /*self*/, PyObject *args)
{
    SelectionLogDisabler disabler(true);
    PyObject *clearPreselect = Py_True;
    char *objname;
    char *docname;
    char* subname = nullptr;
    float x = 0, y = 0, z = 0;
    if (PyArg_ParseTuple(args, "ss|sfffO!", &docname, &objname ,
                &subname,&x,&y,&z,&PyBool_Type,&clearPreselect)) {
        Selection().addSelection(docname, objname, subname, x, y, z, nullptr, Base::asBoolean(clearPreselect));
        Py_Return;
    }

    PyErr_Clear();
    PyObject *object;
    subname = nullptr;
    x = 0, y = 0, z = 0;
    if (PyArg_ParseTuple(args, "O!|sfffO!", &(App::DocumentObjectPy::Type),&object,
                &subname,&x,&y,&z,&PyBool_Type,&clearPreselect)) {
        auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->getNameInDocument()) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
            return nullptr;
        }

        Selection().addSelection(docObj->getDocument()->getName(),
                                 docObj->getNameInDocument(),
                                 subname, x, y, z, nullptr, Base::asBoolean(clearPreselect));
        Py_Return;
    }

    PyErr_Clear();
    PyObject *sequence;
    if (PyArg_ParseTuple(args, "O!O|O!", &(App::DocumentObjectPy::Type),&object,
                &sequence,&PyBool_Type,&clearPreselect))
    {
        auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->getNameInDocument()) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
            return nullptr;
        }

        try {
            if (PyTuple_Check(sequence) || PyList_Check(sequence)) {
                Py::Sequence list(sequence);
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    std::string subname = static_cast<std::string>(Py::String(*it));
                    Selection().addSelection(docObj->getDocument()->getName(),
                                             docObj->getNameInDocument(),
                                             subname.c_str(), 0, 0, 0, nullptr, Base::asBoolean(clearPreselect));
                }
                Py_Return;
            }
        }
        catch (const Py::Exception&) {
            // do nothing here
        }
    }

    PyErr_SetString(PyExc_ValueError, "type must be 'DocumentObject[,subname[,x,y,z]]' or 'DocumentObject, list or tuple of subnames'");

    return nullptr;
}

PyObject *SelectionSingleton::sUpdateSelection(PyObject * /*self*/, PyObject *args)
{
    PyObject *show;
    PyObject *object;
    char* subname=nullptr;
    if(!PyArg_ParseTuple(args, "O!O!|s", &PyBool_Type, &show, &(App::DocumentObjectPy::Type),
            &object, &subname))
        return nullptr;

    auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
    App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
    if (!docObj || !docObj->getNameInDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
        return nullptr;
    }

    Selection().updateSelection(Base::asBoolean(show),
            docObj->getDocument()->getName(), docObj->getNameInDocument(), subname);

    Py_Return;
}


PyObject *SelectionSingleton::sRemoveSelection(PyObject * /*self*/, PyObject *args)
{
    SelectionLogDisabler disabler(true);
    char *docname, *objname;
    char* subname = nullptr;
    if(PyArg_ParseTuple(args, "ss|s", &docname,&objname,&subname)) {
        Selection().rmvSelection(docname,objname,subname);
        Py_Return;
    }

    PyErr_Clear();
    PyObject *object;
    subname = nullptr;
    if (!PyArg_ParseTuple(args, "O!|s", &(App::DocumentObjectPy::Type),&object,&subname))
        return nullptr;

    auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
    App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
    if (!docObj || !docObj->getNameInDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
        return nullptr;
    }

    Selection().rmvSelection(docObj->getDocument()->getName(),
                             docObj->getNameInDocument(),
                             subname);

    Py_Return;
}

PyObject *SelectionSingleton::sClearSelection(PyObject * /*self*/, PyObject *args)
{
    SelectionLogDisabler disabler(true);
    PyObject *clearPreSelect = Py_True;
    char *documentName = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &clearPreSelect)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "|sO!", &documentName, &PyBool_Type, &clearPreSelect))
            return nullptr;
    }
    Selection().clearSelection(documentName, Base::asBoolean(clearPreSelect));

    Py_Return;
}

namespace {
ResolveMode toEnum(int value) {
    switch (value) {
    case 0:
        return ResolveMode::NoResolve;
    case 1:
        return ResolveMode::OldStyleElement;
    case 2:
        return ResolveMode::NewStyleElement;
    case 3:
        return ResolveMode::FollowLink;
    default:
        throw Base::ValueError("Wrong enum value");
    }
}
}

PyObject *SelectionSingleton::sIsSelected(PyObject * /*self*/, PyObject *args)
{
    PyObject *object;
    char* subname = nullptr;
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "O!|si", &(App::DocumentObjectPy::Type), &object, &subname, &resolve))
        return nullptr;

    try {
        auto docObj = static_cast<App::DocumentObjectPy*>(object);
        bool ok = Selection().isSelected(docObj->getDocumentObjectPtr(), subname, toEnum(resolve));

        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

PyObject *SelectionSingleton::sCountObjectsOfType(PyObject * /*self*/, PyObject *args)
{
    char* objecttype;
    char* document = nullptr;
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "s|si", &objecttype, &document,&resolve))
        return nullptr;

    try {
        unsigned int count = Selection().countObjectsOfType(objecttype, document, toEnum(resolve));
        return PyLong_FromLong(count);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

PyObject *SelectionSingleton::sGetSelection(PyObject * /*self*/, PyObject *args)
{
    char *documentName = nullptr;
    int resolve = 1;
    PyObject *single = Py_False;
    if (!PyArg_ParseTuple(args, "|siO!", &documentName, &resolve, &PyBool_Type, &single))
        return nullptr;

    try {
        std::vector<SelectionSingleton::SelObj> sel;
        sel = Selection().getSelection(documentName, toEnum(resolve), Base::asBoolean(single));

        std::set<App::DocumentObject*> noduplicates;
        std::vector<App::DocumentObject*> selectedObjects; // keep the order of selection
        Py::List list;
        for (const auto & it : sel) {
            if (noduplicates.insert(it.pObject).second) {
                selectedObjects.push_back(it.pObject);
            }
        }
        for (const auto & selectedObject : selectedObjects) {
            list.append(Py::asObject(selectedObject->getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject *SelectionSingleton::sEnablePickedList(PyObject * /*self*/, PyObject *args)
{
    PyObject *enable = Py_True;
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &enable))
        return nullptr;

    Selection().enablePickedList(Base::asBoolean(enable));

    Py_Return;
}

PyObject *SelectionSingleton::sSetPreselection(PyObject * /*self*/, PyObject *args, PyObject *kwd)
{
    PyObject *object;
    char* subname = nullptr;
    float x = 0, y = 0, z = 0;
    int type = 1;
    static const std::array<const char *, 7> kwlist{"obj", "subname", "x", "y", "z", "tp", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwd, "O!|sfffi", kwlist, &(App::DocumentObjectPy::Type), &object,
                                            &subname, &x, &y, &z, &type)) {
        auto docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->getNameInDocument()) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check invalid object");
            return nullptr;
        }

        Selection().setPreselect(docObj->getDocument()->getName(),
                                 docObj->getNameInDocument(),
                                 subname,x,y,z, static_cast<SelectionChanges::MsgSource>(type));
        Py_Return;
    }

    PyErr_SetString(PyExc_ValueError, "type must be 'DocumentObject[,subname[,x,y,z]]'");

    return nullptr;
}

PyObject *SelectionSingleton::sGetPreselection(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    const SelectionChanges& sel = Selection().getPreselection();
    SelectionObject obj(sel);

    return obj.getPyObject();
}

PyObject *SelectionSingleton::sRemPreselection(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Selection().rmvPreselect();

    Py_Return;
}

PyObject *SelectionSingleton::sGetCompleteSelection(PyObject * /*self*/, PyObject *args)
{
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "|i",&resolve))
        return nullptr;

    try {
        std::vector<SelectionSingleton::SelObj> sel;
        sel = Selection().getCompleteSelection(toEnum(resolve));

        Py::List list;
        for (const auto & it : sel) {
            SelectionObject obj(SelectionChanges(SelectionChanges::AddSelection,
                                                 it.DocName,
                                                 it.FeatName,
                                                 it.SubName,
                                                 it.TypeName,
                                                 it.x, it.y, it.z));
            list.append(Py::asObject(obj.getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject *SelectionSingleton::sGetSelectionEx(PyObject * /*self*/, PyObject *args)
{
    char *documentName = nullptr;
    int resolve = 1;
    PyObject *single = Py_False;
    if (!PyArg_ParseTuple(args, "|siO!", &documentName, &resolve, &PyBool_Type, &single))
        return nullptr;

    try {
        std::vector<SelectionObject> sel;
        sel = Selection().getSelectionEx(documentName,
                App::DocumentObject::getClassTypeId(), toEnum(resolve), Base::asBoolean(single));

        Py::List list;
        for (auto & it : sel) {
            list.append(Py::asObject(it.getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject *SelectionSingleton::sGetPickedList(PyObject * /*self*/, PyObject *args)
{
    char *documentName = nullptr;
    if (!PyArg_ParseTuple(args, "|s", &documentName))
        return nullptr;

    std::vector<SelectionObject> sel;
    sel = Selection().getPickedListEx(documentName);

    try {
        Py::List list;
        for (auto & it : sel) {
            list.append(Py::asObject(it.getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (Py::Exception&) {
        return nullptr;
    }
}

PyObject *SelectionSingleton::sGetSelectionObject(PyObject * /*self*/, PyObject *args)
{
    char *docName, *objName, *subName;
    PyObject* tuple = nullptr;
    if (!PyArg_ParseTuple(args, "sss|O!", &docName, &objName, &subName, &PyTuple_Type, &tuple))
        return nullptr;

    try {
        SelectionObject selObj;
        selObj.DocName  = docName;
        selObj.FeatName = objName;
        std::string sub = subName;
        if (!sub.empty()) {
            selObj.SubNames.push_back(sub);
            if (tuple) {
                Py::Tuple t(tuple);
                double x = (double)Py::Float(t.getItem(0));
                double y = (double)Py::Float(t.getItem(1));
                double z = (double)Py::Float(t.getItem(2));
                selObj.SelPoses.emplace_back(x,y,z);
            }
        }

        return selObj.getPyObject();
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
}

PyObject *SelectionSingleton::sSetSelectionStyle(PyObject * /*self*/, PyObject *args)
{
    int selStyle = 0;
    if (!PyArg_ParseTuple(args, "i", &selStyle))
        return nullptr;

    PY_TRY {
        Selection().setSelectionStyle(selStyle == 0 ? SelectionStyle::NormalSelection : SelectionStyle::GreedySelection);
        Py_Return;
    }
    PY_CATCH;
}

PyObject *SelectionSingleton::sAddSelObserver(PyObject * /*self*/, PyObject *args)
{
    PyObject* o;
    int resolve = 1;
    if (!PyArg_ParseTuple(args, "O|i", &o, &resolve))
        return nullptr;

    PY_TRY {
        SelectionObserverPython::addObserver(Py::Object(o), toEnum(resolve));
        Py_Return;
    }
    PY_CATCH;
}

PyObject *SelectionSingleton::sRemSelObserver(PyObject * /*self*/, PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o))
        return nullptr;

    PY_TRY {
        SelectionObserverPython::removeObserver(Py::Object(o));
        Py_Return;
    }
    PY_CATCH;
}

PyObject *SelectionSingleton::sAddSelectionGate(PyObject * /*self*/, PyObject *args)
{
    char* filter;
    int resolve = 1;
    if (PyArg_ParseTuple(args, "s|i", &filter, &resolve)) {
        PY_TRY {
            Selection().addSelectionGate(new SelectionFilterGate(filter), toEnum(resolve));
            Py_Return;
        }
        PY_CATCH;
    }

    PyErr_Clear();
    PyObject* filterPy;
    if (PyArg_ParseTuple(args, "O!|i",SelectionFilterPy::type_object(),&filterPy,resolve)) {
        PY_TRY {
            Selection().addSelectionGate(new SelectionFilterGatePython(
                        SelectionFilterPy::cast(filterPy)), toEnum(resolve));
            Py_Return;
        }
        PY_CATCH;
    }

    PyErr_Clear();
    PyObject* gate;
    if (PyArg_ParseTuple(args, "O|i",&gate,&resolve)) {
        PY_TRY {
            Selection().addSelectionGate(new SelectionGatePython(Py::Object(gate, false)), toEnum(resolve));
            Py_Return;
        }
         PY_CATCH;
    }

    PyErr_SetString(PyExc_ValueError, "Argument is neither string nor SelectionFiler nor SelectionGate");

    return nullptr;
}

PyObject *SelectionSingleton::sRemoveSelectionGate(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        Selection().rmvSelectionGate();
        Py_Return;
    }
    PY_CATCH;
}

PyObject *SelectionSingleton::sSetVisible(PyObject * /*self*/, PyObject *args)
{
    PyObject *visible = Py_None;
    if (!PyArg_ParseTuple(args, "|O", &visible))
        return nullptr;

    PY_TRY {
        VisibleState vis = VisToggle;
        Base::PyTypeCheck(&visible, &PyBool_Type);
        if (visible)
            vis = PyObject_IsTrue(visible) ? VisShow : VisHide;

        Selection().setVisible(vis);
        Py_Return;
    }
    PY_CATCH;
}

PyObject *SelectionSingleton::sPushSelStack(PyObject * /*self*/, PyObject *args)
{
    PyObject *clear = Py_True;
    PyObject *overwrite = Py_False;
    if (!PyArg_ParseTuple(args, "|O!O!", &PyBool_Type, &clear, &PyBool_Type, &overwrite))
        return nullptr;

    Selection().selStackPush(Base::asBoolean(clear), Base::asBoolean(overwrite));

    Py_Return;
}

PyObject *SelectionSingleton::sHasSelection(PyObject * /*self*/, PyObject *args)
{
    const char *doc = nullptr;
    int resolve = 0;
    if (!PyArg_ParseTuple(args, "|sO!", &doc, &resolve))
        return nullptr;

    PY_TRY {
        bool ret;
        if (doc || resolve > 0)
            ret = Selection().hasSelection(doc, toEnum(resolve));
        else
            ret = Selection().hasSelection();

        return Py::new_reference_to(Py::Boolean(ret));
    }
    PY_CATCH;
}

PyObject *SelectionSingleton::sHasSubSelection(PyObject * /*self*/, PyObject *args)
{
    const char *doc = nullptr;
    PyObject *subElement = Py_False;
    if (!PyArg_ParseTuple(args, "|sO!",&doc,&PyBool_Type,&subElement))
        return nullptr;

    PY_TRY {
        return Py::new_reference_to(
               Py::Boolean(Selection().hasSubSelection(doc, Base::asBoolean(subElement))));
    }
    PY_CATCH;
}

PyObject *SelectionSingleton::sGetSelectionFromStack(PyObject * /*self*/, PyObject *args)
{
    char *documentName = nullptr;
    int resolve = 1;
    int index = 0;
    if (!PyArg_ParseTuple(args, "|sii", &documentName, &resolve, &index))
        return nullptr;

    PY_TRY {
        Py::List list;
        for(auto &sel : Selection().selStackGet(documentName, toEnum(resolve), index))
            list.append(Py::asObject(sel.getPyObject()));
        return Py::new_reference_to(list);
    }
    PY_CATCH;
}
