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

    HParameterStore copy();

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

    /**
     * @brief redirect: worker behind constrainEqual. Can be used to bypass checks for fixedness.
     * @param who: parameter to be redirected (all from the equality group will be redirected)
     * @param where: parameter to redirect to (auto resolves to master of equality group)
     * @param mean_out: if true, picks the fixed value of the two, or averages out if both are variable.
     */
    void redirect(ParameterRef who, ParameterRef where, bool mean_out = true);
    ///returns list of all parameters redirected (constrained) to one value
    std::vector<ParameterRef> getEqualityGroup(ParameterRef param);

    enum class eConstrainEqual_Result{
        Constrained,
        Redundant
    };
    ///creates redirects to make the parameters equal. Throws if conflicting. It is important that "fixed" fields of related parameters are properly filled for conflict detection.
    eConstrainEqual_Result constrainEqual(ParameterRef param1, ParameterRef param2, bool mean_out = true);
    ///dismantles all equality groups (undoes all calls constrainEqual, except parameter value changes).
    void free();
    ///frees the parameter from equality group (undoes constrainEqual)
    void free(ParameterRef param);
    ///update own values of parameters to match that of equality group (aka its masterValue).
    void sync();
    ///update own value of parameter to match that of equality group (aka its masterValue).
    void sync(ParameterRef param);

    ///marks this parameter as fixed, and makes sure that the equality group it belongs to is fixed too.
    void fix(ParameterRef param);


    PyObject* getPyObject() override;
    HParameterStore getPyHandle() const;

public: //for range-based for looping. Using ParameterRef as an iterator.
    ParameterRef begin();
    ///returns an invalid parameter, only for looping
    ParameterRef end();
public:
    friend class ParameterRef;
};

} //namespace


#endif
