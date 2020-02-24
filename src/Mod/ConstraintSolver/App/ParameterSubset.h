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

#ifndef FREECAD_CONSTRAINTSOLVER_PARAMETERSUBSET_H
#define FREECAD_CONSTRAINTSOLVER_PARAMETERSUBSET_H

#include <vector>
#include "ParameterRef.h"
#include "ParameterStore.h"

namespace FCS {

class ParameterSubset;
typedef UnsafePyHandle<ParameterSubset> HParameterSubset;

class FCSExport ParameterSubset
{
protected://data
    std::vector<ParameterRef> _params;
    ///Lookup table. Input = index of parameter in store. Output = index of parameter in the set.
    std::vector<int> _lut;
    HParameterStore _host;
    PyObject* _twin = nullptr;

protected://methods
    ParameterSubset(int prealloc = 0);
    ParameterSubset(const ParameterSubset& other) = delete;//handle-only, so no copy constructor
    ///called on adding first parameter
    void attach(HParameterStore store);
    void detach();
    void onStoreExpand();
    ///checks if given parameter is from the right store
    bool checkParameter(const ParameterRef& param) const;

public://methods

    //constructors
    static HParameterSubset make(std::vector<ParameterRef> params);
    static HParameterSubset make(HParameterStore store, int prealloc = 0);
    HParameterSubset copy() const;

    //destructors
    ~ParameterSubset();

    HParameterStore host() const;

    int size() const;
    const std::vector<ParameterRef>& list() const {return _params;}

    bool add(ParameterRef param);
    int add(const std::vector<ParameterRef>& params);
    /// remove: removes whole equality group, not just the parameter.
    bool remove(ParameterRef param);
    void clear();
    bool has(ParameterRef param) const;
    bool has(const HParameterSubset other) const;
    bool in(const HParameterSubset other) const;
    /**
     * @brief indexOf
     *
     * @param param: the parameter to look up (feeding parameters that don't
     * belong to the same store can lead to crashes - checks are disabled in
     * release builds. If in doubt - check has() first.).
     *
     * @return index of this parameter in the subset, or -1 if it isn't in the subset.
     */
    int indexOf(const ParameterRef& param) const;

    ParameterRef operator[](int index) const;

    HParameterSubset self() const;
    PyObject* getPyObject();

public://friends
    friend class ParameterStore;
};

} //namespace


#endif
