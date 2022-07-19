/***************************************************************************
 *   Copyright (c) 2017 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "ExceptionFactory.h"
#include <CXX/Objects.hxx>


using namespace Base;

ExceptionFactory* ExceptionFactory::_pcSingleton = nullptr;

ExceptionFactory& ExceptionFactory::Instance()
{
    if (!_pcSingleton)
        _pcSingleton = new ExceptionFactory;
    return *_pcSingleton;
}

void ExceptionFactory::Destruct ()
{
    if (_pcSingleton)
        delete _pcSingleton;
    _pcSingleton = nullptr;
}

void ExceptionFactory::raiseException (PyObject * pydict) const
{
    std::string classname;

    Py::Dict edict(pydict);
    if (edict.hasKey("sclassname")) {
        classname = static_cast<std::string>(Py::String(edict.getItem("sclassname")));

        std::map<const std::string, AbstractProducer*>::const_iterator pProd;

        pProd = _mpcProducers.find(classname.c_str());
        if (pProd != _mpcProducers.end())
            static_cast<AbstractExceptionProducer *>(pProd->second)->raiseException(pydict);
    }
}


