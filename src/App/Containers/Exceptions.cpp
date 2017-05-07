/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)   <vv.titov@gmail.com> 2017     *
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

#include "Exceptions.h"

#include <Base/PyObjectBase.h>
BaseExport extern PyObject* Base::BaseExceptionFreeCADError;

using namespace App;


TYPESYSTEM_SOURCE(App::ContainerError, Base::Exception)

PyObject* App::ContainerError::PyExc = nullptr;

PyObject* App::ContainerError::getPyExcClass() const
{
    return App::ContainerError::PyExc;
}


#define CONTAINER_EXCEPTION_SOURCE(classname) \
    TYPESYSTEM_SOURCE(classname, App::ContainerError) \
    \
    PyObject* classname::PyExc = nullptr;\
    \
    PyObject* classname::getPyExcClass() const \
    { \
        return classname::PyExc; \
    }

CONTAINER_EXCEPTION_SOURCE(App::ContainerTreeError       )
CONTAINER_EXCEPTION_SOURCE(App::AlreadyInContainerError  )
CONTAINER_EXCEPTION_SOURCE(App::ContainerUnsupportedError)
CONTAINER_EXCEPTION_SOURCE(App::RejectedByContainerError )
CONTAINER_EXCEPTION_SOURCE(App::NotAContainerError       )
CONTAINER_EXCEPTION_SOURCE(App::SpecialChildError        )
CONTAINER_EXCEPTION_SOURCE(App::NullContainerError       )
CONTAINER_EXCEPTION_SOURCE(App::ObjectNotFoundError      )


void ContainerError::initContainerExceptionTypes()
{
    ContainerError            ::init();
    ContainerTreeError        ::init();
    AlreadyInContainerError   ::init();
    ContainerUnsupportedError ::init();
    RejectedByContainerError  ::init();
    NotAContainerError        ::init();
    SpecialChildError         ::init();
    NullContainerError        ::init();
    ObjectNotFoundError       ::init();
}

void ContainerError::registerPyExceptions(PyObject* module)
{
    ContainerError::PyExc = PyErr_NewException("App.Containers.ContainerError", Base::BaseExceptionFreeCADError, NULL);
    Py_INCREF(ContainerError::PyExc);
    PyModule_AddObject(module, "ContainerError", ContainerError::PyExc);


#define IMPLEMENT_REGISTER_PY_EXCEPTION(classname) \
    classname::PyExc = PyErr_NewException("App.Containers." #classname, ContainerError::PyExc, NULL); \
    Py_INCREF(classname::PyExc); \
    PyModule_AddObject(module, #classname, classname::PyExc);

    IMPLEMENT_REGISTER_PY_EXCEPTION(ContainerError           );
    IMPLEMENT_REGISTER_PY_EXCEPTION(ContainerTreeError       );
    IMPLEMENT_REGISTER_PY_EXCEPTION(AlreadyInContainerError  );
    IMPLEMENT_REGISTER_PY_EXCEPTION(ContainerUnsupportedError);
    IMPLEMENT_REGISTER_PY_EXCEPTION(RejectedByContainerError );
    IMPLEMENT_REGISTER_PY_EXCEPTION(NotAContainerError       );
    IMPLEMENT_REGISTER_PY_EXCEPTION(SpecialChildError        );
    IMPLEMENT_REGISTER_PY_EXCEPTION(NullContainerError       );
    IMPLEMENT_REGISTER_PY_EXCEPTION(ObjectNotFoundError      );

}




