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


#ifndef BASE_BINDINGMANAGER_H
#define BASE_BINDINGMANAGER_H

#include <memory>
#include <FCGlobal.h>

typedef struct _object PyObject;

namespace Base
{

class BaseExport BindingManager
{
public:
    static BindingManager& instance();

    bool hasWrapper(const void *cptr);

    void registerWrapper(const void* cptr, PyObject* pyObj);
    void releaseWrapper(const void* cptr, PyObject* pyObj);

    PyObject* retrieveWrapper(const void* cptr);

private:
    BindingManager();
    ~BindingManager();

    struct BindingManagerPrivate;
    std::unique_ptr<BindingManagerPrivate> p;
};

}

#endif // BASE_BINDINGMANAGER_H
