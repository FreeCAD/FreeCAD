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

#ifndef FREECAD_CONSTRAINTSOLVER_SKETCHSOLVERSYSTEM_H
#define FREECAD_CONSTRAINTSOLVER_SKETCHSOLVERSYSTEM_H

#include "Utils.h"
#include "ParameterStore.h"
#include "ParameterSubset.h"
#include "ValueSet.h"
#include "Constraint.h"

#include <Eigen/Core>

namespace FCS {

struct Subconstraint
{
public: //data
    HConstraint constraint;
    int index = -1;//index of error function
public: //constructor
    Subconstraint(HConstraint c, int index)
        : constraint(c), index(index) {}
};

class SubSystem;
typedef UnsafePyHandle<SubSystem> HSubSystem;

class SubSystem : public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
protected://data
    PyObject* _twin = nullptr;

    HParameterStore _store;
    HParameterSubset _params;

    std::vector<HConstraint> _constraints;
    std::vector<Subconstraint> _subconstraints;
    std::vector<int> _constraint1stRow;//input: index into _constraints. output: index into _subconstraints
    int _maxConstraintRank = 0; // max(_constraints[].rank())

    HValueSet _curvals; //#FIXME: remove?
    std::map<int,std::vector<Subconstraint> > p2c; //lookup table parameter->constraints

    bool _touched = true;

public: //methods
    SubSystem();
    SubSystem(HParameterSubset params, std::vector<HConstraint> constraints = std::vector<HConstraint>());
    HParameterSubset params() const {return _params;}
    const std::vector<HConstraint>& constraints() const {return _constraints;}
    const std::vector<Subconstraint>& subconstraints(){return _subconstraints;}

    ///fills look-up tables
    void update();
    void touch() {_touched = true;}
    bool isTouched() {return _touched;}

    void addConstraint(HConstraint c);
    void addConstraint(const std::vector<HConstraint>& clist);

    void addUnknown(const ParameterRef& p);
    void addUnknown(HParameterSubset subset);

    ///Jacobi matrix for constraints in this system for arbitrary set of parameters (set can be wider than subset of this subsystem, used for augmented-system solving)
    /**
     * @brief calcJacobi: computes Jacobi matrix (matrix or partial derivatives)
     * @param vals: values of parameters to compute on. Note: vals must include all params
     * @param params: subset of parameters to compute matrix on
     * @param output: the result matrix, must have n_subconstraints rows and params.size() cols.
     */
    void calcJacobi(ValueSet& vals, HParameterSubset params, Eigen::MatrixXd &output);

    void calcJacobi(Eigen::MatrixXd &output);

    ///gradient of total error function
    void calcGrad(HValueSet vals, Eigen::VectorXd &output);

    double maxStep(const ValueSet& vals, const ValueSet& xdir);

    ///computes vector of subconstraint errors, and 0.5*sum of squares of errors
    void calcResidual(const ValueSet& vals, Eigen::VectorXd& output, double& err);
    double error(const ValueSet& vals);

    ///checks if valueset has all the parameters of the subsystem. Throws Py::ValueError if not.
    void checkValuesCoverage(const ValueSet& vals) const;


public://python
    PyObject* getPyObject() override;
    HSubSystem self();

protected://methods

    ~SubSystem() = default; //protected destructor, pyhandle only
    friend class SubSystemPy;
};


} //namespace


#endif
