/***************************************************************************
 *   Copyright (c) 2024 Florian Foinant-Willig <ffw@2f2v.fr>               *
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

#include "DocumentObject.h"

// inclusion of the generated files (generated out of SuppressibleExtensionPy.xml)
#include "SuppressibleExtensionPy.h"
#include "SuppressibleExtensionPy.cpp"
#include "DocumentObjectPy.h"


using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string SuppressibleExtensionPy::representation() const
{
    return {"<suppressible extension object>"};
}

PyObject *SuppressibleExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int SuppressibleExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
