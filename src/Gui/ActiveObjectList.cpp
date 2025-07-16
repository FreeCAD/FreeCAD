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
***************************************************************************/

#include "PreCompiled.h"

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "ActiveObjectList.h"
#include "TreeParams.h"


FC_LOG_LEVEL_INIT("MDIView", true, true)

using namespace Gui;

App::DocumentObject *ActiveObjectList::getObject(const ObjectInfo &info, bool resolve,
                                                 App::DocumentObject **parent,
                                                 std::string *subname) const
{
    if (parent)
        *parent = info.obj;
    if (subname)
        *subname = info.subname;
    auto obj = info.obj;
    if (!obj || !obj->isAttachedToDocument())
        return nullptr;
    if (!info.subname.empty()) {
        obj = obj->getSubObject(info.subname.c_str());
        if (!obj)
            return nullptr;
    }

    return resolve ? obj->getLinkedObject(true) : obj;
}

void ActiveObjectList::setHighlight(const ObjectInfo &info, HighlightMode mode, bool enable)
{
    auto obj = getObject(info, false);
    if (!obj)
        return;
    auto vp = freecad_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(obj));
    if (!vp)
        return;

    if (TreeParams::getTreeActiveAutoExpand()) {
        vp->getDocument()->signalExpandObject(*vp, enable ? TreeItemMode::ExpandPath : TreeItemMode::CollapseItem,
                                              info.obj, info.subname.c_str());
    }

    vp->getDocument()->signalHighlightObject(*vp, mode, enable, info.obj, info.subname.c_str());
}

Gui::ActiveObjectList::ObjectInfo Gui::ActiveObjectList::getObjectInfo(App::DocumentObject *obj, const char *subname) const
{
    ObjectInfo info;
    info.obj = nullptr;
    if (!obj || !obj->isAttachedToDocument())
        return info;

    if (subname) {
        info.obj = obj;
        info.subname = subname;
    }
    else {
        // If the input object is not from this document, it must be brought in
        // by some link type object of this document. We only accept the object
        // if we can find such object in the current selection.
        auto sels = Gui::Selection().getSelection(_Doc->getDocument()->getName(), ResolveMode::NoResolve);
        for (auto &sel : sels) {
            if (sel.pObject == obj || sel.pObject->getLinkedObject(true)==obj) {
                info.obj = sel.pObject;
                break;
            }

            for (auto dot=strchr(sel.SubName,'.');dot;dot=strchr(dot+1,'.')) {
                std::string subname(sel.SubName,dot-sel.SubName+1);
                auto sobj = sel.pObject->getSubObject(subname.c_str());
                if (!sobj)
                    break;
                if (sobj == obj || sobj->getLinkedObject(true) == obj) {
                    info.obj = sel.pObject;
                    info.subname = subname;
                    break;
                }
            }

            if (info.obj)
                break;
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
                if (obj->getDocument()!=_Doc->getDocument())
                    return info;
                info.obj = obj;
            }
        }
    }

    return info;
}

bool Gui::ActiveObjectList::hasObject(App::DocumentObject *obj,
                                      const char *name, const char *subname) const
{
    auto it = _ObjectMap.find(name);
    if (it == _ObjectMap.end())
        return false;
    auto info = getObjectInfo(obj, subname);
    return info.obj == it->second.obj && info.subname == it->second.subname;
}

void Gui::ActiveObjectList::setObject(App::DocumentObject* obj, const char* name,
                                      const char *subname, const Gui::HighlightMode& mode)
{
    auto it = _ObjectMap.find(name);
    if (it!=_ObjectMap.end()) {
        setHighlight(it->second, mode, false);
        _ObjectMap.erase(it);
    }

    if (!obj)
        return;

    auto info = getObjectInfo(obj,subname);
    if (!info.obj) {
        FC_ERR("Cannot set active object "
                << obj->getFullName() << '.' << (subname?subname:"")
                << " in document '" << _Doc->getDocument()->getName()
                << "'. Not found in current selection");
        return;
    }

    _ObjectMap[name] = info;
    setHighlight(info, mode, true);
}

bool Gui::ActiveObjectList::hasObject(const char*name)const
{
    return _ObjectMap.find(name) != _ObjectMap.end();
}

void ActiveObjectList::objectDeleted(const ViewProviderDocumentObject &vp)
{
    // Hint: With C++20 std::erase_if for containers can be used
    auto isEqual = [&vp](const auto& item) {
        return item.second.obj == vp.getObject();
    };
    for (auto it = _ObjectMap.begin(); it != _ObjectMap.end();) {
        if (isEqual(*it)) {
            it = _ObjectMap.erase(it);
        }
        else {
            ++it;
        }
    }
}

App::DocumentObject* ActiveObjectList::getObjectWithExtension(const Base::Type extensionTypeId) const
{
    for (const auto& pair : _ObjectMap) {
        App::DocumentObject* obj = getObject(pair.second, true);
        if (obj && obj->hasExtension(extensionTypeId)) {
            return obj;
        }
    }

    return nullptr;
}
