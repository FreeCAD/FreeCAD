/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
# include <assert.h>
# include <string>
# include <boost/bind.hpp>
# include <QApplication>
# include <QString>
# include <QStatusBar>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Application.h"
#include "Document.h"
#include "Selection.h"
#include "SelectionFilter.h"
#include "View3DInventor.h"
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Gui/SelectionObjectPy.h>
#include "MainWindow.h"

FC_LOG_LEVEL_INIT("Selection",false,true,true)

using namespace Gui;
using namespace std;

SelectionObserver::SelectionObserver() : blockSelection(false)
{
    attachSelection();
}

SelectionObserver::~SelectionObserver()
{
    detachSelection();
}

bool SelectionObserver::blockConnection(bool block)
{
    bool ok = blockSelection;
    if (block)
        blockSelection = true;
    else
        blockSelection = false;
    return ok;
}

bool SelectionObserver::isConnectionBlocked() const
{
    return blockSelection;
}

void SelectionObserver::attachSelection()
{
    if (!connectSelection.connected()) {
        connectSelection = Selection().signalSelectionChanged.connect(boost::bind
            (&SelectionObserver::_onSelectionChanged, this, _1));
    }
}

void SelectionObserver::_onSelectionChanged(const SelectionChanges& msg) {
    try {
        if (blockSelection)
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
    }
}

// -------------------------------------------

std::vector<SelectionObserverPython*> SelectionObserverPython::_instances;

SelectionObserverPython::SelectionObserverPython(const Py::Object& obj) : inst(obj)
{
}

SelectionObserverPython::~SelectionObserverPython()
{
}

void SelectionObserverPython::addObserver(const Py::Object& obj)
{
    _instances.push_back(new SelectionObserverPython(obj));
}

void SelectionObserverPython::removeObserver(const Py::Object& obj)
{
    SelectionObserverPython* obs=0;
    for (std::vector<SelectionObserverPython*>::iterator it =
        _instances.begin(); it != _instances.end(); ++it) {
        if ((*it)->inst == obj) {
            obs = *it;
            _instances.erase(it);
            break;
        }
    }

    delete obs;
}

void SelectionObserverPython::onSelectionChanged(const SelectionChanges& msg)
{
    switch (msg.Type)
    {
    case SelectionChanges::AddSelection:
        addSelection(msg);
        break;
    case SelectionChanges::RmvSelection:
        removeSelection(msg);
        break;
    case SelectionChanges::SetSelection:
        setSelection(msg);
        break;
    case SelectionChanges::ClrSelection:
        clearSelection(msg);
        break;
    case SelectionChanges::SetPreselect:
        setPreselection(msg);
        break;
    case SelectionChanges::RmvPreselect:
        removePreselection(msg);
        break;
    default:
        break;
    }
}

void SelectionObserverPython::addSelection(const SelectionChanges& msg)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("addSelection"))) {
            Py::Callable method(this->inst.getAttr(std::string("addSelection")));
            Py::Tuple args(4);
            args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
            args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
            args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
            Py::Tuple tuple(3);
            tuple[0] = Py::Float(msg.x);
            tuple[1] = Py::Float(msg.y);
            tuple[2] = Py::Float(msg.z);
            args.setItem(3, tuple);
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::removeSelection(const SelectionChanges& msg)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("removeSelection"))) {
            Py::Callable method(this->inst.getAttr(std::string("removeSelection")));
            Py::Tuple args(3);
            args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
            args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
            args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::setSelection(const SelectionChanges& msg)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("setSelection"))) {
            Py::Callable method(this->inst.getAttr(std::string("setSelection")));
            Py::Tuple args(1);
            args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::clearSelection(const SelectionChanges& msg)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("clearSelection"))) {
            Py::Callable method(this->inst.getAttr(std::string("clearSelection")));
            Py::Tuple args(1);
            args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::setPreselection(const SelectionChanges& msg)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("setPreselection"))) {
            Py::Callable method(this->inst.getAttr(std::string("setPreselection")));
            Py::Tuple args(3);
            args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
            args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
            args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::removePreselection(const SelectionChanges& msg)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("removePreselection"))) {
            Py::Callable method(this->inst.getAttr(std::string("removePreselection")));
            Py::Tuple args(3);
            args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
            args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
            args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

// -------------------------------------------

bool SelectionSingleton::hasSelection() const
{
    return !_SelList.empty();
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getCompleteSelection() const
{
    std::vector<SelObj> temp;
    SelObj tempSelObj;

    for(std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        tempSelObj.DocName  = It->DocName.c_str();
        tempSelObj.FeatName = It->FeatName.c_str();
        tempSelObj.SubName  = It->SubName.c_str();
        tempSelObj.TypeName = It->TypeName.c_str();
        tempSelObj.pObject  = It->pObject;
        tempSelObj.pDoc     = It->pDoc;
        temp.push_back(tempSelObj);
    }

    return temp;
}

std::vector<SelectionSingleton::SelObj> SelectionSingleton::getSelection(const char* pDocName) const
{
    std::vector<SelObj> temp;
    SelObj tempSelObj;

    App::Document *pcDoc;
    pcDoc = getDocument(pDocName);

    if (!pcDoc)
        return temp;

    for(std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        if (It->pDoc == pcDoc) {
            tempSelObj.DocName  = It->DocName.c_str();
            tempSelObj.FeatName = It->FeatName.c_str();
            tempSelObj.SubName  = It->SubName.c_str();
            tempSelObj.TypeName = It->TypeName.c_str();
            tempSelObj.pObject  = It->pObject;
            tempSelObj.pDoc     = It->pDoc;
            tempSelObj.x        = It->x;
            tempSelObj.y        = It->y;
            tempSelObj.z        = It->z;
            temp.push_back(tempSelObj);
        }
    }

    return temp;
}

bool SelectionSingleton::hasSelection(const char* doc) const
{
    App::Document *pcDoc;
    pcDoc = getDocument(doc);
    if (!pcDoc)
        return false;
    for(std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        if (It->pDoc == pcDoc) {
            return true;
        }
    }

    return false;
}

std::vector<SelectionObject> SelectionSingleton::getSelectionEx(const char* pDocName, Base::Type typeId) const
{
    std::vector<SelectionObject> temp;
    std::map<App::DocumentObject*,SelectionObject> SortMap;

    // check the type
    if (typeId == Base::Type::badType())
        return temp;

    App::Document *pcDoc;

    pcDoc = getDocument(pDocName);

    if (!pcDoc)
        return temp;

    for (std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        if (It->pDoc == pcDoc) {
            // right type?
            if (It->pObject->getTypeId().isDerivedFrom(typeId)){
                // if the object has already an entry
                if (SortMap.find(It->pObject) != SortMap.end()){
                    // only add sub-element
                    if (!It->SubName.empty()) {
                        SortMap[It->pObject].SubNames.push_back(It->SubName);
                        SortMap[It->pObject].SelPoses.push_back(Base::Vector3d(It->x,It->y,It->z));
                    }
                }
                else {
                    // create a new entry
                    SelectionObject tempSelObj;
                    tempSelObj.DocName  = It->DocName;
                    tempSelObj.FeatName = It->FeatName;
                    tempSelObj.TypeName = It->TypeName.c_str();
                    if (!It->SubName.empty()) {
                        tempSelObj.SubNames.push_back(It->SubName);
                        tempSelObj.SelPoses.push_back(Base::Vector3d(It->x,It->y,It->z));
                    }
                    SortMap.insert(std::pair<App::DocumentObject*,SelectionObject>(It->pObject,tempSelObj));
                }
            }
        }
    }

    // The map loses the order thus we have to go again through the list and pick up the SelectionObject from the map
    for (std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        std::map<App::DocumentObject*,SelectionObject>::iterator Jt = SortMap.find(It->pObject);
        if (Jt != SortMap.end()) {
            temp.push_back(Jt->second);
            SortMap.erase(Jt);
        }
    }

    return temp;
}

int SelectionSingleton::getAsPropertyLinkSubList(App::PropertyLinkSubList &prop) const
{
    std::vector<Gui::SelectionObject> sel = this->getSelectionEx();
    std::vector<App::DocumentObject*> objs; objs.reserve(sel.size()*2);
    std::vector<std::string> subs; subs.reserve(sel.size()*2);
    for (std::size_t iobj = 0; iobj < sel.size(); iobj++) {
        Gui::SelectionObject &selitem = sel[iobj];
        App::DocumentObject* obj = selitem.getObject();
        const std::vector<std::string> &subnames = selitem.getSubNames();
        if (subnames.size() == 0){//whole object is selected
            objs.push_back(obj);
            subs.push_back(std::string());
        } else {
            for (std::size_t isub = 0; isub < subnames.size(); isub++) {
                objs.push_back(obj);
                subs.push_back(subnames[isub]);
            }
        }
    }
    assert(objs.size()==subs.size());
    prop.setValues(objs, subs);
    return objs.size();
}

vector<App::DocumentObject*> SelectionSingleton::getObjectsOfType(const Base::Type& typeId, const char* pDocName) const
{
    std::vector<App::DocumentObject*> temp;
    App::Document *pcDoc;

    pcDoc = getDocument(pDocName);

    if (!pcDoc)
        return temp;

    for (std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        if (It->pDoc == pcDoc && It->pObject && It->pObject->getTypeId().isDerivedFrom(typeId)) {
            temp.push_back(It->pObject);
        }
    }

    return temp;
}

std::vector<App::DocumentObject*> SelectionSingleton::getObjectsOfType(const char* typeName, const char* pDocName) const
{
    Base::Type typeId = Base::Type::fromName(typeName);
    if (typeId == Base::Type::badType())
        return std::vector<App::DocumentObject*>();
    return getObjectsOfType(typeId, pDocName);
}

unsigned int SelectionSingleton::countObjectsOfType(const Base::Type& typeId, const char* pDocName) const
{
    unsigned int iNbr=0;
    App::Document *pcDoc;

    pcDoc = getDocument(pDocName);

    if (!pcDoc)
        return 0;

    for (std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        if (It->pDoc == pcDoc && It->pObject && It->pObject->getTypeId().isDerivedFrom(typeId)) {
            iNbr++;
        }
    }

    return iNbr;
}

unsigned int SelectionSingleton::countObjectsOfType(const char* typeName, const char* pDocName) const
{
    Base::Type typeId = Base::Type::fromName(typeName);
    if (typeId == Base::Type::badType())
        return 0;
    return countObjectsOfType(typeId, pDocName);
}

bool SelectionSingleton::setPreselect(const char* pDocName, const char* pObjectName, const char* pSubName, float x, float y, float z)
{
    if (DocName != "")
        rmvPreselect();

    if (ActiveGate) {
        // if document of object doesn't exist then return with false
        App::Document* pDoc = getDocument(pDocName);
        if (!pDoc || !pObjectName)
            return false;
        App::DocumentObject* pObject = pDoc->getObject(pObjectName);
        if (!pObject)
            return false;

        if (!ActiveGate->allow(pDoc,pObject,pSubName)) {
            QString msg;
            if (ActiveGate->notAllowedReason.length() > 0){
                msg = QObject::tr(ActiveGate->notAllowedReason.c_str());
            } else {
                msg = QCoreApplication::translate("SelectionFilter","Not allowed:");
            }
            msg += QString::fromUtf8(" "); 
            msg.append(
                        QObject::tr("%1.%2.%3")
                       .arg(QString::fromLatin1(pDocName))
                       .arg(QString::fromLatin1(pObjectName))
                       .arg(QString::fromLatin1(pSubName))
                        );
            msg += QString::fromUtf8(" "); 

            if (getMainWindow()) {
                getMainWindow()->showMessage(msg);
                Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                mdi->setOverrideCursor(QCursor(Qt::ForbiddenCursor));
            }
            return false;
        }
    }

    DocName = pDocName;
    FeatName= pObjectName;
    SubName = pSubName;
    hx = x;
    hy = y;
    hz = z;

    // set up the change object
    SelectionChanges Chng;
    Chng.pDocName  = DocName.c_str();
    Chng.pObjectName = FeatName.c_str();
    Chng.pSubName  = SubName.c_str();
    Chng.pTypeName = "";
    Chng.x = x;
    Chng.y = y;
    Chng.z = z;
    Chng.Type = SelectionChanges::SetPreselect;

    // set the current preselection
    CurrentPreselection = Chng;

    Notify(Chng);
    signalSelectionChanged(Chng);

    // allows the preselection
    return true;
}

void SelectionSingleton::setPreselectCoord( float x, float y, float z)
{
    static char buf[513];

    // if nothing is in preselect ignore
    if(!CurrentPreselection.pObjectName) return;

    CurrentPreselection.x = x;
    CurrentPreselection.y = y;
    CurrentPreselection.z = z;

    snprintf(buf,512,"Preselected: %s.%s.%s (%f,%f,%f)",CurrentPreselection.pDocName
                                                       ,CurrentPreselection.pObjectName
                                                       ,CurrentPreselection.pSubName
                                                       ,x,y,z);

    if (getMainWindow())
        getMainWindow()->showMessage(QString::fromLatin1(buf));
}

void SelectionSingleton::rmvPreselect()
{
    if (DocName == "")
        return;

    SelectionChanges Chng;
    Chng.pDocName  = DocName.c_str();
    Chng.pObjectName = FeatName.c_str();
    Chng.pSubName  = SubName.c_str();
    Chng.Type = SelectionChanges::RmvPreselect;

    // reset the current preselection
    CurrentPreselection.pDocName =0;
    CurrentPreselection.pObjectName = 0;
    CurrentPreselection.pSubName = 0;
    CurrentPreselection.x = 0.0;
    CurrentPreselection.y = 0.0;
    CurrentPreselection.z = 0.0;

    // notify observing objects
    Notify(Chng);
    signalSelectionChanged(Chng);

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

    //Base::Console().Log("Sel : Rmv preselect \n");
}

const SelectionChanges &SelectionSingleton::getPreselection(void) const
{
    return CurrentPreselection;
}

// add a SelectionGate to control what is selectable
void SelectionSingleton::addSelectionGate(Gui::SelectionGate *gate)
{
    if (ActiveGate)
        rmvSelectionGate();

    ActiveGate = gate;
}

// remove the active SelectionGate
void SelectionSingleton::rmvSelectionGate(void)
{
    if (ActiveGate) {
        delete ActiveGate;
        ActiveGate=0;
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc) {
            Gui::MDIView* mdi = doc->getActiveView();
            mdi->restoreOverrideCursor();
        }
    }
}


App::Document* SelectionSingleton::getDocument(const char* pDocName) const
{
    if (pDocName)
        return App::GetApplication().getDocument(pDocName);
    else
        return App::GetApplication().getActiveDocument();
}

bool SelectionSingleton::addSelection(const char* pDocName, const char* pObjectName, const char* pSubName, float x, float y, float z)
{
    // already in ?
    if (isSelected(pDocName, pObjectName, pSubName))
        return true;

    _SelObj temp;

    temp.pDoc = getDocument(pDocName);

    if (temp.pDoc) {
        if(pObjectName)
            temp.pObject = temp.pDoc->getObject(pObjectName);
        else
            temp.pObject = 0;

        // check for a Selection Gate
        if (ActiveGate) {
            if (!ActiveGate->allow(temp.pDoc,temp.pObject,pSubName)) {
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

        temp.DocName  = pDocName;
        temp.FeatName = pObjectName ? pObjectName : "";
        temp.SubName  = pSubName ? pSubName : "";
        temp.x        = x;
        temp.y        = y;
        temp.z        = z;

        if (temp.pObject)
            temp.TypeName = temp.pObject->getTypeId().getName();

        _SelList.push_back(temp);

        SelectionChanges Chng;

        Chng.pDocName  = pDocName;
        Chng.pObjectName = pObjectName ? pObjectName : "";
        Chng.pSubName  = pSubName ? pSubName : "";
        Chng.pTypeName = temp.TypeName.c_str();
        Chng.x         = x;
        Chng.y         = y;
        Chng.z         = z;
        Chng.Type      = SelectionChanges::AddSelection;


        Notify(Chng);
        signalSelectionChanged(Chng);

#ifdef FC_DEBUG
        Base::Console().Log("Sel : Add Selection \"%s.%s.%s(%f,%f,%f)\"\n",pDocName,pObjectName,pSubName,x,y,z);
#endif

        // allow selection
        return true;
    }
    else {
        // neither an existing nor active document available
        // this can often happen when importing .iv files
        Base::Console().Error("Cannot add to selection: no document '%s' found.\n", pDocName);
        return false;
    }
}

bool SelectionSingleton::addSelection(const char* pDocName, const char* pObjectName, const std::vector<std::string>& pSubNames)
{
    _SelObj temp;

    temp.pDoc = getDocument(pDocName);

    if (temp.pDoc) {
        if(pObjectName)
            temp.pObject = temp.pDoc->getObject(pObjectName);
        else
            temp.pObject = 0;

        if (temp.pObject)
            temp.TypeName = temp.pObject->getTypeId().getName();

        temp.DocName  = pDocName;
        temp.FeatName = pObjectName ? pObjectName : "";
        if (!pSubNames.empty()) {
            for (std::vector<std::string>::const_iterator it = pSubNames.begin(); it != pSubNames.end(); ++it) {
                temp.SubName  = it->c_str();
                temp.x        = 0;
                temp.y        = 0;
                temp.z        = 0;

                _SelList.push_back(temp);
            }
        }
        else {
            temp.SubName  = "";
            temp.x        = 0;
            temp.y        = 0;
            temp.z        = 0;
            _SelList.push_back(temp);
        }

        SelectionChanges Chng;

        Chng.pDocName  = pDocName;
        Chng.pObjectName = pObjectName ? pObjectName : "";
        Chng.pSubName  = "";
        Chng.pTypeName = temp.TypeName.c_str();
        Chng.x         = 0;
        Chng.y         = 0;
        Chng.z         = 0;
        Chng.Type      = SelectionChanges::AddSelection;

        Notify(Chng);
        signalSelectionChanged(Chng);

        // allow selection
        return true;
    }
    else {
        // neither an existing nor active document available
        // this can often happen when importing .iv files
        Base::Console().Error("Cannot add to selection: no document '%s' found.\n", pDocName);
        return false;
    }
}

bool SelectionSingleton::addSelection(const SelectionObject& obj)
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
                               static_cast<float>(pnt.z));
        }
        return ok;
    }
    else if (!subNames.empty()) {
        bool ok = true;
        for (std::size_t i=0; i<subNames.size(); i++) {
            const std::string& name = subNames[i];
            ok &= addSelection(obj.getDocName(), obj.getFeatName(), name.c_str());
        }
        return ok;
    }
    else {
        return addSelection(obj.getDocName(), obj.getFeatName());
    }
}

void SelectionSingleton::rmvSelection(const char* pDocName, const char* pObjectName, const char* pSubName)
{
    bool foundSelection = false;
    std::string tmpDocName;
    std::string tmpFeaName;
    std::string tmpSubName;
    std::string tmpTypName;

    for (std::list<_SelObj>::iterator It = _SelList.begin();It != _SelList.end();) {
        if ((It->DocName == pDocName && !pObjectName) ||
            (It->DocName == pDocName && pObjectName && It->FeatName == pObjectName && !pSubName) ||
            (It->DocName == pDocName && pObjectName && It->FeatName == pObjectName && pSubName && It->SubName == pSubName))
        {
            // save in tmp. string vars
            tmpDocName = It->DocName;
            tmpFeaName = It->FeatName;
            tmpSubName = It->SubName;
            tmpTypName = It->TypeName;

            // destroy the _SelObj item
            It = _SelList.erase(It);

            foundSelection = true;

#ifdef FC_DEBUG
            Base::Console().Log("Sel : Rmv Selection \"%s.%s.%s\"\n",pDocName,pObjectName,pSubName);
#endif
        }
        else {
            ++It;
        }
    }

    // NOTE: It can happen that there are nested calls of rmvSelection()
    // so that it's not safe to invoke the notifications inside the loop
    // as this can invalidate the iterators and thus leads to undefined
    // behaviour.
    // So, the notification is done after the loop, see also #0003469
    if (foundSelection) {
        SelectionChanges Chng;
        Chng.pDocName  = tmpDocName.c_str();
        Chng.pObjectName = tmpFeaName.c_str();
        Chng.pSubName  = tmpSubName.c_str();
        Chng.pTypeName = tmpTypName.c_str();
        Chng.Type      = SelectionChanges::RmvSelection;
        Notify(Chng);
        signalSelectionChanged(Chng);
    }
}

void SelectionSingleton::setSelection(const char* pDocName, const std::vector<App::DocumentObject*>& sel)
{
    App::Document *pcDoc;
    pcDoc = getDocument(pDocName);
    if (!pcDoc)
        return;

    std::set<App::DocumentObject*> cur_sel, new_sel;
    new_sel.insert(sel.begin(), sel.end());

    // Make sure to keep the order of the currently selected objects
    std::list<_SelObj> temp;
    for (std::list<_SelObj>::const_iterator it = _SelList.begin(); it != _SelList.end(); ++it) {
        if (it->pDoc != pcDoc)
            temp.push_back(*it);
        else {
            cur_sel.insert(it->pObject);
            if (new_sel.find(it->pObject) != new_sel.end())
                temp.push_back(*it);
        }
    }

    // Get the objects we must add to the selection
    std::vector<App::DocumentObject*> diff_new_cur;
    std::back_insert_iterator< std::vector<App::DocumentObject*> > biit(diff_new_cur);
    std::set_difference(new_sel.begin(), new_sel.end(), cur_sel.begin(), cur_sel.end(), biit);

    _SelObj obj;
    for (std::vector<App::DocumentObject*>::const_iterator it = diff_new_cur.begin(); it != diff_new_cur.end(); ++it) {
        obj.pDoc = pcDoc;
        obj.pObject = *it;
        obj.DocName = pDocName;
        obj.FeatName = (*it)->getNameInDocument();
        obj.SubName = "";
        obj.TypeName = (*it)->getTypeId().getName();
        obj.x = 0.0f;
        obj.y = 0.0f;
        obj.z = 0.0f;
        temp.push_back(obj);
    }

    if (cur_sel == new_sel) // nothing has changed
        return;

    _SelList = temp;

    SelectionChanges Chng;
    Chng.Type = SelectionChanges::SetSelection;
    Chng.pDocName = pDocName;
    Chng.pObjectName = "";
    Chng.pSubName = "";
    Chng.pTypeName = "";

    Notify(Chng);
    signalSelectionChanged(Chng);
}

void SelectionSingleton::clearSelection(const char* pDocName)
{
    App::Document* pDoc;
    pDoc = getDocument(pDocName);

    // the document 'pDocName' has already been removed
    if (!pDoc && !pDocName) {
        clearCompleteSelection();
    }
    else {
        std::string docName;
        if (pDocName)
            docName = pDocName;
        else
            docName = pDoc->getName(); // active document
        std::list<_SelObj> selList;
        for (std::list<_SelObj>::iterator it = _SelList.begin(); it != _SelList.end(); ++it) {
            if (it->DocName != docName)
                selList.push_back(*it);
        }

        _SelList = selList;

        SelectionChanges Chng;
        Chng.Type = SelectionChanges::ClrSelection;
        Chng.pDocName = docName.c_str();
        Chng.pObjectName = "";
        Chng.pSubName = "";
        Chng.pTypeName = "";

        Notify(Chng);
        signalSelectionChanged(Chng);

#ifdef FC_DEBUG
        Base::Console().Log("Sel : Clear selection\n");
#endif
    }
}

void SelectionSingleton::clearCompleteSelection()
{
    _SelList.clear();

    SelectionChanges Chng;
    Chng.Type = SelectionChanges::ClrSelection;
    Chng.pDocName = "";
    Chng.pObjectName = "";
    Chng.pSubName = "";
    Chng.pTypeName = "";


    Notify(Chng);
    signalSelectionChanged(Chng);

#ifdef FC_DEBUG
    Base::Console().Log("Sel : Clear selection\n");
#endif
}

bool SelectionSingleton::isSelected(const char* pDocName, const char* pObjectName, const char* pSubName) const
{
    const char* tmpDocName = pDocName ? pDocName : "";
    const char* tmpFeaName = pObjectName ? pObjectName : "";
    const char* tmpSubName = pSubName ? pSubName : "";
    for (std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It)
        if (It->DocName == tmpDocName && It->FeatName == tmpFeaName && It->SubName == tmpSubName)
            return true;
    return false;
}

bool SelectionSingleton::isSelected(App::DocumentObject* obj, const char* pSubName) const
{
    if (!obj) return false;

    for(list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        if (It->pObject == obj) {
            if (pSubName) {
                if (It->SubName == pSubName)
                    return true;
            }
            else {
                return true;
            }
        }
    }

    return false;
}

void SelectionSingleton::slotDeletedObject(const App::DocumentObject& Obj)
{
    // remove also from the selection, if selected
    Selection().rmvSelection( Obj.getDocument()->getName(), Obj.getNameInDocument() );
}


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
SelectionSingleton::SelectionSingleton()
{
    hx = 0;
    hy = 0;
    hz = 0;
    ActiveGate = 0;
    App::GetApplication().signalDeletedObject.connect(boost::bind(&Gui::SelectionSingleton::slotDeletedObject, this, _1));
    CurrentPreselection.Type = SelectionChanges::ClrSelection;
    CurrentPreselection.pDocName = 0;
    CurrentPreselection.pObjectName = 0;
    CurrentPreselection.pSubName = 0;
    CurrentPreselection.pTypeName = 0;
    CurrentPreselection.x = 0.0;
    CurrentPreselection.y = 0.0;
    CurrentPreselection.z = 0.0;
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
SelectionSingleton::~SelectionSingleton()
{
}

SelectionSingleton* SelectionSingleton::_pcSingleton = NULL;

SelectionSingleton& SelectionSingleton::instance(void)
{
    if (_pcSingleton == NULL)
        _pcSingleton = new SelectionSingleton;
    return *_pcSingleton;
}

void SelectionSingleton::destruct (void)
{
    if (_pcSingleton != NULL)
        delete _pcSingleton;
    _pcSingleton = 0;
}

//**************************************************************************
// Python stuff

// SelectionSingleton Methods  // Methods structure
PyMethodDef SelectionSingleton::Methods[] = {
    {"addSelection",         (PyCFunction) SelectionSingleton::sAddSelection, METH_VARARGS,
     "addSelection(object,[string,float,float,float]) -- Add an object to the selection\n"
     "where string is the sub-element name and the three floats represent a 3d point"},
    {"removeSelection",      (PyCFunction) SelectionSingleton::sRemoveSelection, METH_VARARGS,
     "removeSelection(object) -- Remove an object from the selection"},
    {"clearSelection"  ,     (PyCFunction) SelectionSingleton::sClearSelection, METH_VARARGS,
     "clearSelection([string]) -- Clear the selection\n"
     "Clear the selection to the given document name. If no document is\n"
     "given the complete selection is cleared."},
    {"isSelected",           (PyCFunction) SelectionSingleton::sIsSelected, METH_VARARGS,
     "isSelected(object) -- Check if a given object is selected"},
    {"getPreselection",      (PyCFunction) SelectionSingleton::sGetPreselection, METH_VARARGS,
     "getPreselection() -- Get preselected object"},
    {"clearPreselection",   (PyCFunction) SelectionSingleton::sRemPreselection, METH_VARARGS,
     "clearPreselection() -- Clear the preselection"},
    {"countObjectsOfType",   (PyCFunction) SelectionSingleton::sCountObjectsOfType, METH_VARARGS,
     "countObjectsOfType(string, [string]) -- Get the number of selected objects\n"
     "The first argument defines the object type e.g. \"Part::Feature\" and the\n"
     "second argumeht defines the document name. If no document name is given the\n"
     "currently active document is used"},
    {"getSelection",         (PyCFunction) SelectionSingleton::sGetSelection, METH_VARARGS,
     "getSelection([string]) -- Return a list of selected objects\n"
     "Return a list of selected objects for a given document name. If no\n"
     "document name is given the selection for the active document is returned."},
    {"getCompleteSelection", (PyCFunction) SelectionSingleton::sGetCompleteSelection, METH_VARARGS,
     "getCompleteSelection() -- Return a list of selected objects of all documents."},
    {"getSelectionEx",         (PyCFunction) SelectionSingleton::sGetSelectionEx, METH_VARARGS,
     "getSelectionEx([string]) -- Return a list of SelectionObjects\n"
     "Return a list of SelectionObjects for a given document name. If no\n"
     "document is given the selection of the active document is returned.\n"
     "The SelectionObjects contain a variety of information about the selection,\n"
     "e.g. sub-element names."},
    {"getSelectionObject",  (PyCFunction) SelectionSingleton::sGetSelectionObject, METH_VARARGS,
     "getSelectionObject(doc,obj,sub,(x,y,z)) -- Return a SelectionObject"},
    {"addObserver",         (PyCFunction) SelectionSingleton::sAddSelObserver, METH_VARARGS,
     "addObserver(Object) -- Install an observer\n"},
    {"removeObserver",      (PyCFunction) SelectionSingleton::sRemSelObserver, METH_VARARGS,
     "removeObserver(Object) -- Uninstall an observer\n"},
    {"addSelectionGate",      (PyCFunction) SelectionSingleton::sAddSelectionGate, METH_VARARGS,
     "addSelectionGate(String|Filter|Gate) -- activate the selection gate.\n"
     "The selection gate will prohibit all selections which do not match\n"
     "the given selection filter string.\n"
     " Examples strings are:\n"
     "'SELECT Part::Feature SUBELEMENT Edge',\n"
     "'SELECT Robot::RobotObject'\n"
     "\n"
     "You can also set an instance of SelectionFilter:\n"
     "filter = Gui.Selection.Filter('SELECT Part::Feature SUBELEMENT Edge')\n"
     "Gui.Selection.addSelectionGate(filter)\n"
     "\n"
     "And the most flexible approach is to write your own selection gate class\n"
     "that implements the method 'allow'\n"
     "class Gate:\n"
     "  def allow(self,doc,obj,sub):\n"
     "    return (sub[0:4] == 'Face')\n"
     "Gui.Selection.addSelectionGate(Gate())"},
    {"removeSelectionGate",      (PyCFunction) SelectionSingleton::sRemoveSelectionGate, METH_VARARGS,
     "removeSelectionGate() -- remove the active selection gate\n"},
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

PyObject *SelectionSingleton::sAddSelection(PyObject * /*self*/, PyObject *args)
{
    PyObject *object;
    char* subname=0;
    float x=0,y=0,z=0;
    if (PyArg_ParseTuple(args, "O!|sfff", &(App::DocumentObjectPy::Type),&object,&subname,&x,&y,&z)) {
        App::DocumentObjectPy* docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->getNameInDocument()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Cannot check invalid object");
            return NULL;
        }

        Selection().addSelection(docObj->getDocument()->getName(),
                                 docObj->getNameInDocument(),
                                 subname,x,y,z);
        Py_Return;
    }

    PyErr_Clear();
    PyObject *sequence;
    if (PyArg_ParseTuple(args, "O!O", &(App::DocumentObjectPy::Type),&object,&sequence)) {
        App::DocumentObjectPy* docObjPy = static_cast<App::DocumentObjectPy*>(object);
        App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
        if (!docObj || !docObj->getNameInDocument()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Cannot check invalid object");
            return NULL;
        }

        try {
            if (PyTuple_Check(sequence) || PyList_Check(sequence)) {
                Py::Sequence list(sequence);
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    std::string subname = static_cast<std::string>(Py::String(*it));
                    Selection().addSelection(docObj->getDocument()->getName(),
                                             docObj->getNameInDocument(),
                                             subname.c_str());
                }

                Py_Return;
            }
        }
        catch (const Py::Exception&) {
            // do nothing here
        }
    }

    PyErr_SetString(PyExc_ValueError, "type must be 'DocumentObject[,subname[,x,y,z]]' or 'DocumentObject, list or tuple of subnames'");
    return 0;
}

PyObject *SelectionSingleton::sRemoveSelection(PyObject * /*self*/, PyObject *args)
{
    PyObject *object;
    char* subname=0;
    if (!PyArg_ParseTuple(args, "O!|s", &(App::DocumentObjectPy::Type),&object,&subname))
        return NULL;

    App::DocumentObjectPy* docObjPy = static_cast<App::DocumentObjectPy*>(object);
    App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
    if (!docObj || !docObj->getNameInDocument()) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, "Cannot check invalid object");
        return NULL;
    }

    Selection().rmvSelection(docObj->getDocument()->getName(),
                             docObj->getNameInDocument(),
                             subname);

    Py_Return;
}

PyObject *SelectionSingleton::sClearSelection(PyObject * /*self*/, PyObject *args)
{
    char *documentName=0;
    if (!PyArg_ParseTuple(args, "|s", &documentName))
        return NULL;
    documentName ? Selection().clearSelection(documentName) : Selection().clearCompleteSelection();
    Py_Return;
}

PyObject *SelectionSingleton::sIsSelected(PyObject * /*self*/, PyObject *args)
{
    PyObject *object;
    char* subname=0;
    if (!PyArg_ParseTuple(args, "O!|s", &(App::DocumentObjectPy::Type), &object, &subname))
        return NULL;

    App::DocumentObjectPy* docObj = static_cast<App::DocumentObjectPy*>(object);
    bool ok = Selection().isSelected(docObj->getDocumentObjectPtr(), subname);
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject *SelectionSingleton::sCountObjectsOfType(PyObject * /*self*/, PyObject *args)
{
    char* objecttype;
    char* document=0;
    if (!PyArg_ParseTuple(args, "s|s", &objecttype, &document))
        return NULL;

    unsigned int count = Selection().countObjectsOfType(objecttype, document);
#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong(count);
#else
    return PyLong_FromLong(count);
#endif
}

PyObject *SelectionSingleton::sGetSelection(PyObject * /*self*/, PyObject *args)
{
    char *documentName=0;
    if (!PyArg_ParseTuple(args, "|s", &documentName))
        return NULL;

    std::vector<SelectionSingleton::SelObj> sel;
    sel = Selection().getSelection(documentName);

    try {
        std::set<App::DocumentObject*> noduplicates;
        Py::List list;
        for (std::vector<SelectionSingleton::SelObj>::iterator it = sel.begin(); it != sel.end(); ++it) {
            noduplicates.insert(it->pObject);
        }
        for (std::set<App::DocumentObject*>::iterator it = noduplicates.begin(); it != noduplicates.end(); ++it) {
            list.append(Py::asObject((*it)->getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (Py::Exception&) {
        return 0;
    }
}

PyObject *SelectionSingleton::sGetPreselection(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    const SelectionChanges& sel = Selection().getPreselection();
    SelectionObject obj(sel);
    return obj.getPyObject();
}

PyObject *SelectionSingleton::sRemPreselection(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Selection().rmvPreselect();
    Py_Return;
}

PyObject *SelectionSingleton::sGetCompleteSelection(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    std::vector<SelectionSingleton::SelObj> sel;
    sel = Selection().getCompleteSelection();

    try {
        Py::List list;
        for (std::vector<SelectionSingleton::SelObj>::iterator it = sel.begin(); it != sel.end(); ++it) {
            list.append(Py::asObject(it->pObject->getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (Py::Exception&) {
        return 0;
    }
}

PyObject *SelectionSingleton::sGetSelectionEx(PyObject * /*self*/, PyObject *args)
{
    char *documentName=0;
    if (!PyArg_ParseTuple(args, "|s", &documentName))
        return NULL;

    std::vector<SelectionObject> sel;
    sel = Selection().getSelectionEx(documentName);

    try {
        Py::List list;
        for (std::vector<SelectionObject>::iterator it = sel.begin(); it != sel.end(); ++it) {
            list.append(Py::asObject(it->getPyObject()));
        }
        return Py::new_reference_to(list);
    }
    catch (Py::Exception&) {
        return 0;
    }
}

PyObject *SelectionSingleton::sGetSelectionObject(PyObject * /*self*/, PyObject *args)
{
    char *docName, *objName, *subName;
    PyObject* tuple=0;
    if (!PyArg_ParseTuple(args, "sss|O!", &docName, &objName, &subName,
                                          &PyTuple_Type, &tuple))
        return NULL;

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
                selObj.SelPoses.push_back(Base::Vector3d(x,y,z));
            }
        }

        return selObj.getPyObject();
    }
    catch (const Py::Exception&) {
        return 0;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
}

PyObject *SelectionSingleton::sAddSelObserver(PyObject * /*self*/, PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        SelectionObserverPython::addObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}

PyObject *SelectionSingleton::sRemSelObserver(PyObject * /*self*/, PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        SelectionObserverPython::removeObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}

PyObject *SelectionSingleton::sAddSelectionGate(PyObject * /*self*/, PyObject *args)
{
    char* filter;
    if (PyArg_ParseTuple(args, "s",&filter)) {
        PY_TRY {
            Selection().addSelectionGate(new SelectionFilterGate(filter));
            Py_Return;
        } PY_CATCH;
    }

    PyErr_Clear();
    PyObject* filterPy;
    if (PyArg_ParseTuple(args, "O!",SelectionFilterPy::type_object(),&filterPy)) {
        PY_TRY {
            Selection().addSelectionGate(new SelectionFilterGatePython(static_cast<SelectionFilterPy*>(filterPy)));
            Py_Return;
        } PY_CATCH;
    }

    PyErr_Clear();
    PyObject* gate;
    if (PyArg_ParseTuple(args, "O",&gate)) {
        PY_TRY {
            Selection().addSelectionGate(new SelectionGatePython(Py::Object(gate, false)));
            Py_Return;
        } PY_CATCH;
    }

    PyErr_SetString(PyExc_ValueError, "Argument is neither string nor SelectionFiler nor SelectionGate");
    return 0;
}

PyObject *SelectionSingleton::sRemoveSelectionGate(PyObject * /*self*/, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        Selection().rmvSelectionGate();
    } PY_CATCH;

    Py_Return;
}
