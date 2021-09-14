/**************************************************************************
*   Copyright (c) 2014 Jürgen Riegel <juergen.riegel@web.de>              *
*                                                                         *
*   This file is part of the FreeCAD CAx development system.              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Library General Public License (LGPL)   *
*   as published by the Free Software Foundation; either version 2 of     *
*   the License, or (at your option) any later version.                   *
*   for detail see the LICENCE text file.                                 *
*                                                                         *
*   FreeCAD is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU Library General Public License for more details.                  *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with FreeCAD; if not, write to the Free Software        *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
*   USA                                                                   *
*                                                                         *
*   Juergen Riegel 2014                                                   *
***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_

#endif

#include <Base/Console.h>
#include "ActiveObjectList.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>
#include "TreeParams.h"

FC_LOG_LEVEL_INIT("MDIView",true,true)

using namespace Gui;

ActiveObjectList::ActiveObjectList(Document *doc)
    :_Doc(doc)
{
    connChangedChildren = Application::Instance->signalChangedChildren.connect(
        [this](const ViewProviderDocumentObject &) {
            if (_ObjectMap.size())
                timer.start(10);
        });
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [this]() {
        for (auto it = _ObjectMap.begin(); it!=_ObjectMap.end();) {
            if (!_getObject(it->second))
                it = _ObjectMap.erase(it);
            else
                ++it;
        }
    });
}

App::DocumentObject *ActiveObjectList::_getObject(ObjectInfo &info,
                                                  App::DocumentObject **parent,
                                                  std::string *subname) const
{
    if(parent) *parent = info.obj;
    if(subname) *subname = info.subname;
    auto obj = info.obj;
    if(info.subname.size()) {
        obj = obj->getSubObject(info.subname.c_str());
        if (obj)
            obj = obj->getLinkedObject(true);
    } else if (obj->getParents().size())
        obj = nullptr;
    if (obj != info.activeObject) {
        setHighlight(info, false);
        auto newInfo = getObjectInfo(info.activeObject, 0);
        if (newInfo.activeObject == nullptr)
            return nullptr;
        newInfo.mode = info.mode;
        info = newInfo;
        if(parent) *parent = info.obj;
        if(subname) *subname = info.subname;
        setHighlight(info, true);
    }

    return info.activeObject;
}

void ActiveObjectList::setHighlight(const ObjectInfo &info, bool enable) const
{
    auto vp = dynamic_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(info.activeObject));
    if(!vp) return;

    if (TreeParams::TreeActiveAutoExpand()) {
        vp->getDocument()->signalExpandObject(*vp, enable ? TreeItemMode::ExpandPath : TreeItemMode::CollapseItem,
                                              info.obj, info.subname.c_str());
    }

    Gui::Application::Instance->signalHighlightObject(*vp, info.mode,enable,info.obj,info.subname.c_str());
}

Gui::ActiveObjectList::ObjectInfo Gui::ActiveObjectList::getObjectInfo(App::DocumentObject *obj, const char *subname) const
{
    ObjectInfo info;
    info.obj = 0;
    info.activeObject = 0;
    info.mode = HighlightMode::UserDefined;
    if(!obj || !obj->getNameInDocument())
        return info;
    bool checkSelection = true;
    if(subname) {
        info.obj = obj;
        info.subname = subname;
        checkSelection = false;
    }else{
        // No subname is given, try Selection().getContext() first
        auto ctxobj = Gui::Selection().getContext().getSubObject();
        if (ctxobj && (ctxobj == obj || ctxobj->getLinkedObject(true) == obj)) {
            info.obj = Gui::Selection().getContext().getObject();
            if (info.obj->getDocument() == _Doc->getDocument()) {
                info.subname = Gui::Selection().getContext().getSubName();
                checkSelection = false;
            }
        }
    }

    if (checkSelection) {
        // If the input object is not from this document, it must be brought in
        // by some link type object of this document. We only accept the object
        // if we can find such object in the current selection.
        auto sels = Gui::Selection().getSelection(_Doc->getDocument()->getName(),false);
        for(auto &sel : sels) {
            if(sel.pObject == obj || sel.pObject->getLinkedObject(true)==obj) {
                info.obj = sel.pObject;
                break;
            }
            for(auto dot=strchr(sel.SubName,'.');dot;dot=strchr(dot+1,'.')) {
                std::string subname(sel.SubName,dot-sel.SubName+1);
                auto sobj = sel.pObject->getSubObject(subname.c_str());
                if(!sobj) break;
                if(sobj == obj || sobj->getLinkedObject(true) == obj) {
                    info.obj = sel.pObject;
                    info.subname = subname;
                    break;
                }
            }
            if(info.obj) break;
        }
        if(!info.obj) {
            // No selection is found, try to obtain the object hierarchy using
            // DocumentObject::getParents()
            unsigned long count = 0xffffffff;
            for(auto &v : obj->getParents()) {
                if(v.first->getDocument() != _Doc->getDocument())
                    continue;

                // We prioritize on non-linked group object having the least
                // hierarchies.
                unsigned long cnt = v.first->getSubObjectList(v.second.c_str()).size();
                if(v.first->getLinkedObject(false) != v.first)
                    cnt &= 0x8000000;
                if(cnt < count) {
                    count = cnt;
                    info.obj = v.first;
                    info.subname = v.second;
                }
            }

            if(!info.obj) {
                if (obj->getDocument()==_Doc->getDocument())
                    info.obj = obj;
                return info;
            }
        }
    }

    if (auto sobj = info.obj->getSubObject(info.subname.c_str()))
        info.activeObject = sobj->getLinkedObject(true);
    return info;
}

bool Gui::ActiveObjectList::hasObject(App::DocumentObject *obj,
        const char *name, const char *subname) const
{
    auto it = _ObjectMap.find(name);
    if(it==_ObjectMap.end())
        return false;
    auto info = getObjectInfo(obj,subname);
    if (!subname)
        return info.activeObject == obj;
    return info.obj == it->second.obj
        && info.subname == it->second.subname
        && info.activeObject == it->second.activeObject;
}

void Gui::ActiveObjectList::setObject(App::DocumentObject* obj, const char* name,
        const char *subname, const Gui::HighlightMode& mode)
{
    auto it = _ObjectMap.find(name);
    if(it!=_ObjectMap.end()) {
        setHighlight(it->second,false);
        _ObjectMap.erase(it);
    }
    if(!obj) return;

    auto info = getObjectInfo(obj,subname);
    if(!info.activeObject) {
        FC_ERR("Cannot set active object "
                << obj->getFullName() << '.' << (subname?subname:"")
                << " in document '" << _Doc->getDocument()->getName()
                << "'. Not found in current selection");
        return;
    }

    if (mode != HighlightMode::None)
        info.mode = mode;
    _ObjectMap[name] = info;
    setHighlight(info,true);
}

bool Gui::ActiveObjectList::hasObject(const char*name)const
{
    return _ObjectMap.find(name) != _ObjectMap.end();
}

void ActiveObjectList::objectDeleted(const ViewProviderDocumentObject &vp)
{
  //maybe boost::bimap or boost::multi_index
  for (auto it = _ObjectMap.begin(); it != _ObjectMap.end(); ++it)
  {
    if (it->second.obj == vp.getObject() || it->second.activeObject == vp.getObject())
    {
      _ObjectMap.erase(it);
      return;
    }
  }
}
