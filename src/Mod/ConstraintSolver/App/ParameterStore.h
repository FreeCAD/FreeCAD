/***************************************************************************
 *   Copyright (c) 2019 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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
#pragma once //to make qt creator happy, see QTCREATORBUG-20883

#ifndef FREECAD_CONSTRAINTSOLVER_PARAMETERSTORE_H
#define FREECAD_CONSTRAINTSOLVER_PARAMETERSTORE_H

namespace GCS {
class ParameterStore;
}

#include "Utils.h"
#include "Parameter.h"
#include "ParameterRef.h"
//#include <Console.h> //DEBUG

namespace GCS {


class ParameterRef;
class ParameterSet;
typedef UnsafePyHandle<ParameterStore> HParameterStore;


class GCSExport ParameterStore : public Base::BaseClass {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
protected: //data
    std::vector<Parameter> _params;
    PyObject* _twin;
protected: //methods
    ParameterStore(int prealloc = 0); //protect all constructors because handle-only memory management
    ///fills indexes for newly created parameters
    void on_added(int old_sz, int new_sz);
public:
    ///the constructor
    static HParameterStore make(int prealloc = 0);
    ParameterStore(const ParameterStore&) = delete; //delete copy constructor because handle-only memory management
    virtual ~ParameterStore();

    HParameterStore copy(HParameterStore other);

    ///adds one new parameter and returns a reference to it
    ParameterRef add();
    ///Adds a new parameter and initializes it. The supplied Parameter instance is copied.
    ParameterRef add(const Parameter& p);
    std::vector<ParameterRef> add(int count);
    std::vector<ParameterRef> add(const std::vector<Parameter>& pp);

    int size() const;
    void resize(int newSize);

    ParameterRef operator[](int index) const;

    double& value(int index);
    double value(int index) const;

    PyObject* getPyObject() override {return _twin;}
    HParameterStore getPyHandle() const;
public:
    friend class ParameterRef;
};

} //namespace


#endif
