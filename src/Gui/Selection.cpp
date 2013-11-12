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
# include <boost/signals.hpp>
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
#include "SelectionObjectPy.h"
#include "View3DInventor.h"
#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include "MainWindow.h"



using namespace Gui;
using namespace std;

SelectionObserver::SelectionObserver()
{
    attachSelection();
}

SelectionObserver::~SelectionObserver()
{
    detachSelection();
}

bool SelectionObserver::blockConnection(bool block)
{
    bool ok = connectSelection.blocked();
    if (block)
        connectSelection.block();
    else
        connectSelection.unblock();
    return ok;
}

bool SelectionObserver::isConnectionBlocked() const
{
    return connectSelection.blocked();
}

void SelectionObserver::attachSelection()
{
    if (!connectSelection.connected()) {
        connectSelection = Selection().signalSelectionChanged.connect(boost::bind
            (&SelectionObserver::onSelectionChanged, this, _1));
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
    string DocName;

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

    // The map looses the order thus we have to go again through the list and pick up the SelectionObject from the map
    for (std::list<_SelObj>::const_iterator It = _SelList.begin();It != _SelList.end();++It) {
        std::map<App::DocumentObject*,SelectionObject>::iterator Jt = SortMap.find(It->pObject);
        if (Jt != SortMap.end()) {
            temp.push_back(Jt->second);
            SortMap.erase(Jt);
        }
    }

    return temp;
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
    static char buf[513];

    if (DocName != "")
        rmvPreselect();

    if (ActiveGate) {
        App::Document* pDoc = getDocument(pDocName);
        if (pDoc) {
            if (pObjectName) {
                App::DocumentObject* pObject = pDoc->getObject(pObjectName);
                if (!ActiveGate->allow(pDoc,pObject,pSubName)) {
                    snprintf(buf,512,"Not allowed: %s.%s.%s ",pDocName
                                                       ,pObjectName
                                                       ,pSubName
                                                       );

                    if (getMainWindow()) {
                        getMainWindow()->showMessage(QString::fromAscii(buf),3000);
                        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                        mdi->setOverrideCursor(QCursor(Qt::ForbiddenCursor));
                    }
                    return false;
                }

            }
            else
                return ActiveGate->allow(pDoc,0,0);
        }
        else
            return false;

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
    Chng.x = x;
    Chng.y = y;
    Chng.z = z;
    Chng.Type = SelectionChanges::SetPreselect;

    // set the current preselection
    CurrentPreselection = Chng;

    snprintf(buf,512,"Preselected: %s.%s.%s (%f,%f,%f)",Chng.pDocName
                                                       ,Chng.pObjectName
                                                       ,Chng.pSubName
                                                       ,x,y,z);

    //FIXME: We shouldn't replace the possibly defined edit cursor
    //with the arrow cursor. But it seems that we don't even have to.
    //if (getMainWindow()){
    //    getMainWindow()->showMessage(QString::fromAscii(buf),3000);
    //    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    //    mdi->restoreOverrideCursor();
    //}

    Notify(Chng);
    signalSelectionChanged(Chng);

    //Base::Console().Log("Sel : Add preselect %s \n",pObjectName);

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
        getMainWindow()->showMessage(QString::fromAscii(buf),3000);
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
                    getMainWindow()->showMessage(QString::fromAscii("Selection not allowed by filter"),5000);
                    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                    mdi->setOverrideCursor(Qt::ForbiddenCursor);
                }
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
        Chng.x         = x;
        Chng.y         = y;
        Chng.z         = z;
        Chng.Type      = SelectionChanges::AddSelection;


        Notify(Chng);
        signalSelectionChanged(Chng);

        Base::Console().Log("Sel : Add Selection \"%s.%s.%s(%f,%f,%f)\"\n",pDocName,pObjectName,pSubName,x,y,z);

        // allow selection
        return true;
    }
    else { // neither an existing nor active document available 
        //assert(0);
        // this can often happen when importing .iv files
        Base::Console().Error("Cannot add to selection: no document '%s' found.\n", pDocName);
        return false;
    }
}

void SelectionSingleton::rmvSelection(const char* pDocName, const char* pObjectName, const char* pSubName)
{
    std::vector<SelectionChanges> rmvList;

    for (std::list<_SelObj>::iterator It = _SelList.begin();It != _SelList.end();) {
        if ((It->DocName == pDocName && !pObjectName) ||
            (It->DocName == pDocName && pObjectName && It->FeatName == pObjectName && !pSubName) ||
            (It->DocName == pDocName && pObjectName && It->FeatName == pObjectName && pSubName && It->SubName == pSubName))
        {
            // save in tmp. string vars
            std::string tmpDocName = It->DocName;
            std::string tmpFeaName = It->FeatName;
            std::string tmpSubName = It->SubName;

            // destroy the _SelObj item
            It = _SelList.erase(It);

            SelectionChanges Chng;
            Chng.pDocName  = tmpDocName.c_str();
            Chng.pObjectName = tmpFeaName.c_str();
            Chng.pSubName  = tmpSubName.c_str();
            Chng.Type      = SelectionChanges::RmvSelection;

            Notify(Chng);
            signalSelectionChanged(Chng);
      
            rmvList.push_back(Chng);
            Base::Console().Log("Sel : Rmv Selection \"%s.%s.%s\"\n",pDocName,pObjectName,pSubName);
        }
        else {
            ++It;
        }
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

        Notify(Chng);
        signalSelectionChanged(Chng);

        Base::Console().Log("Sel : Clear selection\n");
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


    Notify(Chng);
    signalSelectionChanged(Chng);

    Base::Console().Log("Sel : Clear selection\n");
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

void SelectionSingleton::slotRenamedObject(const App::DocumentObject& Obj)
{
    // compare internals with the document and change them if needed
    App::Document* pDoc = Obj.getDocument();
    for (std::list<_SelObj>::iterator it = _SelList.begin(); it != _SelList.end(); ++it) {
        if (it->pDoc == pDoc) {
            it->DocName = pDoc->getName();
        }
    }
}


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
SelectionSingleton::SelectionSingleton()
{
    ActiveGate = 0;
    App::GetApplication().signalDeletedObject.connect(boost::bind(&Gui::SelectionSingleton::slotDeletedObject, this, _1));
    App::GetApplication().signalRenamedObject.connect(boost::bind(&Gui::SelectionSingleton::slotRenamedObject, this, _1));
    CurrentPreselection.pDocName = 0;
    CurrentPreselection.pObjectName = 0;
    CurrentPreselection.pSubName = 0;

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
/*
void SelectionSingleton::addObject(App::DocumentObject *f)
{
  _ObjectSet.insert(f);

}

void SelectionSingleton::removeObject(App::DocumentObject *f)
{
  _ObjectSet.erase(f);


}
*/


//**************************************************************************
// Python stuff

// SelectionSingleton Methods  // Methods structure
PyMethodDef SelectionSingleton::Methods[] = {
    {"addSelection",         (PyCFunction) SelectionSingleton::sAddSelection, 1, 
     "addSelection(object) -- Add an object to the selection"},
    {"removeSelection",      (PyCFunction) SelectionSingleton::sRemoveSelection, 1,
     "removeSelection(object) -- Remove an object from the selection"},
    {"clearSelection"  ,     (PyCFunction) SelectionSingleton::sClearSelection, 1,
     "clearSelection([string]) -- Clear the selection\n"
     "Clear the selection to the given document name. If no document is\n"
     "given the complete selection is cleared."},
    {"isSelected",           (PyCFunction) SelectionSingleton::sIsSelected, 1,
     "isSelected(object) -- Check if a given object is selected"},
    {"countObjectsOfType",   (PyCFunction) SelectionSingleton::sCountObjectsOfType, 1,
     "countObjectsOfType(string, [string]) -- Get the number of selected objects\n"
     "The first argument defines the object type e.g. \"Part::Feature\" and the\n"
     "second argumeht defines the document name. If no document name is given the\n"
     "currently active document is used"},
    {"getSelection",         (PyCFunction) SelectionSingleton::sGetSelection, 1,
     "getSelection([string]) -- Return a list of selected objets\n"
     "Return a list of selected objects for a given document name. If no\n"
     "document is given the complete selection is returned."},
    {"getSelectionEx",         (PyCFunction) SelectionSingleton::sGetSelectionEx, 1,
     "getSelectionEx([string]) -- Return a list of SelectionObjects\n"
     "Return a list of SelectionObjects for a given document name. If no\n"
     "document is given the selection of the active document is returned.\n"
     "The SelectionObjects contain a variety of information about the selection,\n"
     "e.g. sub-element names."},
    {"getSelectionObject",  (PyCFunction) SelectionSingleton::sGetSelectionObject, 1,
     "getSelectionObject(doc,obj,sub,(x,y,z)) -- Return a SelectionObject"},
    {"addObserver",         (PyCFunction) SelectionSingleton::sAddSelObserver, 1,
     "addObserver(Object) -- Install an observer\n"},
    {"removeObserver",      (PyCFunction) SelectionSingleton::sRemSelObserver, 1,
     "removeObserver(Object) -- Uninstall an observer\n"},
    {"addSelectionGate",      (PyCFunction) SelectionSingleton::sAddSelectionGate, 1,
    "addSelectionGate(String) -- activate the selection gate.\n"
    "The selection gate will prohibit all selections which do not match\n"
    "the given selection filter string. Examples strings are:\n"
    "'SELECT Part::Feature SUBELEMENT Edge',\n"
    "'SELECT Robot::RobotObject'\n"},
    {"removeSelectionGate",      (PyCFunction) SelectionSingleton::sRemoveSelectionGate, 1,
     "removeSelectionGate() -- remove the active slection gate\n"},
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

PyObject *SelectionSingleton::sAddSelection(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    PyObject *object;
    char* subname=0;
    float x=0,y=0,z=0;
    if (!PyArg_ParseTuple(args, "O!|sfff", &(App::DocumentObjectPy::Type),&object,&subname,&x,&y,&z))
        return NULL;                             // NULL triggers exception 

    App::DocumentObjectPy* docObjPy = static_cast<App::DocumentObjectPy*>(object);
    App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
    if (!docObj || !docObj->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check invalid object");
        return NULL;
    }

    Selection().addSelection(docObj->getDocument()->getName(),
                             docObj->getNameInDocument(),
                             subname,x,y,z);

    Py_Return;
}

PyObject *SelectionSingleton::sRemoveSelection(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    PyObject *object;
    char* subname=0;
    if (!PyArg_ParseTuple(args, "O!|s", &(App::DocumentObjectPy::Type),&object,&subname))
        return NULL;                             // NULL triggers exception 

    App::DocumentObjectPy* docObjPy = static_cast<App::DocumentObjectPy*>(object);
    App::DocumentObject* docObj = docObjPy->getDocumentObjectPtr();
    if (!docObj || !docObj->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check invalid object");
        return NULL;
    }

    Selection().rmvSelection(docObj->getDocument()->getName(),
                             docObj->getNameInDocument(),
                             subname);

    Py_Return;
}

PyObject *SelectionSingleton::sClearSelection(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    char *documentName=0;
    if (!PyArg_ParseTuple(args, "|s", &documentName))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception
    documentName ? Selection().clearSelection(documentName) : Selection().clearCompleteSelection();
    Py_Return;
}

PyObject *SelectionSingleton::sIsSelected(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    PyObject *object;
    char* subname=0;
    if (!PyArg_ParseTuple(args, "O!|s", &(App::DocumentObjectPy::Type), &object, &subname))
        return NULL;                             // NULL triggers exception 

    App::DocumentObjectPy* docObj = static_cast<App::DocumentObjectPy*>(object);
    bool ok = Selection().isSelected(docObj->getDocumentObjectPtr(), subname);
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject *SelectionSingleton::sCountObjectsOfType(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    char* objecttype;
    char* document=0;
    if (!PyArg_ParseTuple(args, "s|s", &objecttype, &document))
        return NULL;

    unsigned int count = Selection().countObjectsOfType(objecttype, document);
    return PyInt_FromLong(count);
}

PyObject *SelectionSingleton::sGetSelection(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    char *documentName=0;
    if (!PyArg_ParseTuple(args, "|s", &documentName))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception

    std::vector<SelectionSingleton::SelObj> sel;
    if (documentName)
        sel = Selection().getSelection(documentName);
    else
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

PyObject *SelectionSingleton::sGetSelectionEx(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    char *documentName=0;
    if (!PyArg_ParseTuple(args, "|s", &documentName))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception

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

PyObject *SelectionSingleton::sGetSelectionObject(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
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
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
}

PyObject *SelectionSingleton::sAddSelObserver(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        SelectionObserverPython::addObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}

PyObject *SelectionSingleton::sRemSelObserver(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O",&o))
        return NULL;
    PY_TRY {
        SelectionObserverPython::removeObserver(Py::Object(o));
        Py_Return;
    } PY_CATCH;
}

PyObject *SelectionSingleton::sAddSelectionGate(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    char* filter;
    if (!PyArg_ParseTuple(args, "s",&filter))
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        Selection().addSelectionGate(new SelectionFilterGate(filter));
    } PY_CATCH;

    Py_Return;
}

PyObject *SelectionSingleton::sRemoveSelectionGate(PyObject * /*self*/, PyObject *args, PyObject * /*kwd*/)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;                             // NULL triggers exception 

    PY_TRY {
        Selection().rmvSelectionGate();
    } PY_CATCH;

    Py_Return;
}
