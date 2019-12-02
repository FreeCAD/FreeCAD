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

namespace FCS {

class ValueSet;

typedef UnsafePyHandle<ValueSet> HValueSet ;
/**
 * @brief ValueSet stores overrides for parameter values. Use [] operator (with
 * ParameterRef as an index) to obtain the overridden value of a parameter.
 *
 * It is based on Eigen::VectorXd, and can be used in place of one.
 */
class FCSExport ValueSet
{
protected://data
    Eigen::VectorXd _values;
    std::vector<double> _duals;
    std::vector<double> _scales;
    HParameterSubset _subset;
    PyObject* _twin = nullptr;
protected://methods
    ValueSet(HParameterSubset subset);
    ValueSet(HParameterSubset subset, const Eigen::VectorXd& vals, bool no_size_check);
    ValueSet(HParameterSubset subset, const ValueSet& other);
    ValueSet(const ValueSet& other) = delete; //handle-only, so no copy constructor

    void init(bool skip_values = false);
    void checkSameSize(int sz);
    void checkSameSet(const ValueSet& other);
public:
    //constructors
    static HValueSet make(HParameterSubset subset);
    static HValueSet make(HParameterSubset subset, const Eigen::VectorXd& vals, bool no_size_check = false);
    ///makes a new empty ValueSet, for evaluation of geometry and constraints outside of solver.
    static HValueSet makeTrivial(HParameterStore store);
    ///makes a valueset and inits values from another valueset
    static HValueSet makeFrom(HParameterSubset subset, const ValueSet& other);
    ///makes a valueset with all values initialized to zero (aimed at things like direction information for maxStep)
    static HValueSet makeZeros(HParameterSubset subset);
    HValueSet copy() const;

    int size() const;

    const ParameterSubset& subset() const;
    Eigen::VectorXd& values() {return _values;}
    const Eigen::VectorXd& values() const {return _values;}
    std::vector<double>& duals() {return _duals;}
    const std::vector<double>& duals() const {return _duals;}

    ///returns values as saved in parameters (doesn't change this)
    Eigen::VectorXd savedValues() const;
    ///returns a vector of values fetched from another valueSet (not on same parameterset)
    Eigen::VectorXd getSubvector(const ParameterSubset& params) const;
    ///reset values to saved values
    void reset();
    void initFrom(const ValueSet& from);
    ///save values to parameters
    void apply() const;
    ///write only overridden values of "from" into this valueset.
    void paste(const ValueSet& from);
    void paste(const HValueSet from);

    ///set up duals for computing derivatives by a parameter. Returns true if assigned, false if the parameter is not in the set.
    bool setForDerivative(ParameterRef param);
    ///set up duals for computing derivatives along a direction
    void setForDerivative(const Eigen::VectorXd& dir);
    void resetDerivatives();
    ///assumes the parameter is in the set!
    void setDual(const ParameterRef& param, double val);

    ///accepts any parameter from store.
    Base::DualNumber operator[](const ParameterRef& param) const;
    Base::DualNumber operator[](int index) const;
    ///with checks against parameters from different stores
    Base::DualNumber get(const ParameterRef& param) const;


    HValueSet self() const;
public://operators
    operator Eigen::VectorXd&() {return _values;}
    operator const Eigen::VectorXd&() const {return _values;}
    void operator=(const ValueSet& other);
    void operator=(const HValueSet& other);
    void operator=(const Eigen::VectorXd& vals);
    void operator+=(const ValueSet& other);
    void operator*=(double mult);
    //define all on-handle operators. They are convenient: we have to return a handle. Having them avoids extra dereference when chaining operations
    friend HValueSet operator+(const HValueSet &a, const HValueSet &b);
    friend HValueSet operator+(const HValueSet &a, const Eigen::VectorXd &b);
    friend HValueSet operator+(const Eigen::VectorXd &a, const HValueSet &b);
    friend HValueSet operator-(const HValueSet &a, const HValueSet &b);
    friend HValueSet operator-(const HValueSet &a, const Eigen::VectorXd &b);
    friend HValueSet operator-(const Eigen::VectorXd &a, const HValueSet &b);
    friend HValueSet operator*(const HValueSet &a, double b);
    friend HValueSet operator*(double b, const HValueSet &a);
    friend HValueSet operator/(const HValueSet &a, double b);
};

//define further operators between combinations of hanlde, reference, and eigen-vector
inline HValueSet operator+(const ValueSet& a, const ValueSet& b)        {return a.self() + b.self();}
inline HValueSet operator+(const HValueSet& a, const ValueSet& b)       {return a        + b.self();}
inline HValueSet operator+(const ValueSet& a, const HValueSet& b)       {return a.self() + b;}
inline HValueSet operator+(const ValueSet& a, const Eigen::VectorXd& b) {return a.self() + b;}
inline HValueSet operator+(const Eigen::VectorXd& a, const ValueSet& b) {return a        + b.self();}

inline HValueSet operator-(const ValueSet& a, const ValueSet& b)        {return a.self() - b.self();}
inline HValueSet operator-(const HValueSet& a, const ValueSet& b)       {return a        - b.self();}
inline HValueSet operator-(const ValueSet& a, const HValueSet& b)       {return a.self() - b;}
inline HValueSet operator-(const ValueSet& a, const Eigen::VectorXd& b) {return a.self() - b;}
inline HValueSet operator-(const Eigen::VectorXd& a, const ValueSet& b) {return a        - b.self();}

inline HValueSet operator*(const ValueSet& a, double b){return a.self() * b;}
inline HValueSet operator*(double b, const ValueSet& a){return a.self() * b;}
inline HValueSet operator/(const ValueSet& a, double b){return a.self() / b;}


} //namespace


#endif
