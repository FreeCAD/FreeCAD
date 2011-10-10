/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de) 2008                        *
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
# ifdef FC_OS_WIN32
#  include <Rpc.h>
# endif
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Uuid.h"
#include "Exception.h"
#include "Interpreter.h"
#include <CXX/Objects.hxx>


using namespace Base;


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Uuid::Uuid()
{
    UuidStr = CreateUuid();
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Uuid::~Uuid()
{
}

//**************************************************************************
//**************************************************************************
// Get the UUID
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

std::string Uuid::CreateUuid(void)
{
#ifdef FC_OS_WIN32
    RPC_STATUS rstat;
    UUID uuid;
    unsigned char *uuidStr;

    rstat = UuidCreate(&uuid);
    if (rstat != RPC_S_OK) throw Base::Exception("Cannot convert a unique Windows UUID to a string");

    rstat = UuidToString(&uuid, &uuidStr);
    if (rstat != RPC_S_OK) throw Base::Exception("Cannot convert a unique Windows UUID to a string");

    std::string Uuid((char *)uuidStr);

    /* convert it from rcp memory to our own */
    //container = nssUTF8_Duplicate(uuidStr, NULL);
    RpcStringFree(&uuidStr);
#else
    // use Python's implemententation
    std::string Uuid;
    PyGILStateLocker lock;
    try {
        Py::Module module(PyImport_ImportModule("uuid"),true);
        Py::Callable method(module.getAttr("uuid4"));
        Py::Tuple arg;
        Py::Object guid = method.apply(arg);
        Uuid = guid.as_string();
    }
    catch (Py::Exception& e) {
        e.clear();
        throw Base::Exception("Creation of UUID failed");
    }
#endif 
    return Uuid;
}
