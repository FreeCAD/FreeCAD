/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <unordered_map>
#endif

#include "BindingManager.h"

using namespace Base;

struct BindingManager::BindingManagerPrivate {
    std::unordered_map<const void*, PyObject*> wrapperMapper;

    bool hasWrapper(const void *cptr)
    {
        auto it = wrapperMapper.find(cptr);
        return it != wrapperMapper.end();
    }

    void registerWrapper(const void* cptr, PyObject* pyObj)
    {
        wrapperMapper[cptr] = pyObj;
    }

    void releaseWrapper(const void* cptr, PyObject* pyObj)
    {
        auto it = wrapperMapper.find(cptr);
        if (it != wrapperMapper.end() && (!pyObj || it->second == pyObj)) {
            wrapperMapper.erase(it);
        }
    }

    PyObject* retrieveWrapper(const void* cptr)
    {
        auto it = wrapperMapper.find(cptr);
        if (it != wrapperMapper.end()) {
            return it->second;
        }

        return nullptr;
    }
};

BindingManager& BindingManager::instance()
{
    static BindingManager singleton;
    return singleton;
}

BindingManager::BindingManager()
    : p(new BindingManagerPrivate)
{
}

BindingManager::~BindingManager() = default;

bool BindingManager::hasWrapper(const void *cptr)
{
    return p->hasWrapper(cptr);
}

void BindingManager::registerWrapper(const void* cptr, PyObject* pyObj)
{
    p->registerWrapper(cptr, pyObj);
}

void BindingManager::releaseWrapper(const void* cptr, PyObject* pyObj)
{
    p->releaseWrapper(cptr, pyObj);
}

PyObject* BindingManager::retrieveWrapper(const void* cptr)
{
    return p->retrieveWrapper(cptr);
}
