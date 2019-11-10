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

#ifndef FREECAD_CONSTRAINTSOLVER_VALUESET_H
#define FREECAD_CONSTRAINTSOLVER_VALUESET_H

#include "ParameterRef.h"
#include "ParameterStore.h"
#include "ParameterSubset.h"
#include "Eigen/QR"
#include <Base/DualNumber.h>

namespace GCS {

class ValueSet;

typedef UnsafePyHandle<ValueSet> HValueSet;

/**
 * @brief ValueSet stores overrides for parameter values. Use [] operator (with
 * ParameterRef as an index) to obtain the overridden value of a parameter.
 *
 * It is based on Eigen::VectorXd, and can be used in place of one.
 */
class GCSExport ValueSet
{
protected://data
    Eigen::VectorXd _values;
    std::vector<double> _duals;
    std::vector<double> _scales;
    HParameterSubset _subset;
    PyObject* _twin;
protected://methods
    ValueSet(HParameterSubset subset);
    ValueSet(const ValueSet& other) = delete; //handle-only, so no copy constructor

public:
    //constructors
    static HValueSet make(HParameterSubset subset);
    ///makes a new empty ValueSet, for evaluation of geometry and constraints outside of solver.
    static HValueSet makeTrivial(HParameterStore store);
    HValueSet copy() const;

    int size() const;

    const ParameterSubset& subset() const;
    Eigen::VectorXd& values() {return _values;}
    const Eigen::VectorXd& values() const {return _values;}
    std::vector<double>& duals() {return _duals;}
    const std::vector<double>& duals() const {return _duals;}

    ///returns values as saved in parameters (doesn't change this)
    Eigen::VectorXd savedValues() const;
    ///reset values to saved values
    void reset();
    ///save values to parameters
    void apply() const;

    ///set up duals for computing derivatives by a parameter. Returns true if assigned, false if the parameter is not in the set.
    bool setForDerivative(ParameterRef param);
    ///set up duals for computing derivatives along a direction
    void setForDerivative(const Eigen::VectorXd& dir);

    ///accepts any parameter from store.
    Base::DualNumber operator[](ParameterRef param) const;
    Base::DualNumber operator[](int index) const;
    ///with checks against parameters from different stores
    Base::DualNumber get(ParameterRef param) const;
    operator Eigen::VectorXd&() {return _values;}
    operator const Eigen::VectorXd&() const {return _values;}

    HValueSet self() const;
};

} //namespace


#endif
