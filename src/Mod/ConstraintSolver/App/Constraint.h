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

#ifndef FREECAD_CONSTRAINTSOLVER_CONSTRAINT_H
#define FREECAD_CONSTRAINTSOLVER_CONSTRAINT_H

#include "ParaObject.h"
#include "ParameterRef.h"
#include "ValueSet.h"
#include "Utils.h"

#include <Base/BaseClass.h>

#include <vector>

namespace FCS {

class Constraint;
typedef UnsafePyHandle<Constraint> HConstraint;

/**
 * @brief The Constraint class is base for all constraints. Constraints can be
 * multidimensional - i.e., they make rank()-many error values, and remove
 * rank() degrees of freedom. If your constraint doesn't need it, consider overriding SimpleConstraint.
 */
class FCSExport Constraint : public ParaObject
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //enums
    enum class ePriority{
        Normal,
        Low,
    };

protected://data members
    ///user-modified weight of the constraint
    double _weight = 1.0;
    ///constraint reversal multiplier. 1.0 for nonreversed, -1.0 for reversed.
    double _revers = 1.0;
    ProblemSizeInfo _sz;

public://data members
    /**
     * @brief scale is used by solver: the return from the error function is
     * multiplied internally by the scale. You must set the scale from
     * implementation of setWeight(). If you have an error function that returns
     * mm, it is a good idea to set the scale to
     *
     * weight * 1/(size_of_element),
     *
     * so that the constraint doesn't blow up its weight
     * compared to angle constraints when the geometry is scaled up. Do it in
     * implementation of setProblemSize.
     */
    double scale = 1.0;
    ePriority priority;

public://main interface for overriding
    //mandatory
    virtual PyObject* getPyObject() override = 0;
    ///returns number of DOFs taken away by the constraint.
    virtual int rank() const = 0;
    /**
     * @brief error: the error function (with implicit derivative calculation)
     * @param on: the values to calculate the constraint on. Use on[my_param] to get the value of the parameter as a dual number.
     * @param returnbuf: output. The error function must write rank() signed error values into the buffer.
     *
     * DualNumbers are used to implicitly calculate derivatives. Typically, one of the parameters gets a dual value of 1.0 (scaled by parameter's scale), while the rest have 0.0 as their duals. However, several parameters at once may have duals at 1.0 thanks to redirection. Also, all parameters may have nonzero duals, for calculating derivatives along a vector.
     *
     * As long as you never downcast a dualnumber to a double, the derivatives should be calculated implicitly, and you don't have to worry about them.
     */
    virtual void error(const ValueSet& vals, Base::DualNumber* returnbuf) const = 0;
    //optional
    virtual double maxStep(const ValueSet& vals, const ValueSet& dir) const;
    virtual void setWeight(double weight);
    ///reversedness of the constraint. The meaning is constraint-dependent.
    bool isReversed() const {return _revers < 0.0;}
    virtual void setReversed(bool brev);
    virtual void setProblemSize(ProblemSizeInfo sz);
    ///datum parameters are usually numerals entered by user, such as the distance of distance constraint. The parameters are typically fixed.
    virtual std::vector<ParameterRef> datumParameters() const;
    ///returns calculated values for datum parameters for current state of geometry. Same order as returned by datumParameters.
    virtual std::vector<Base::DualNumber> caluclateDatum(const ValueSet& vals);

public://methods
    const std::vector<ParameterRef>& parameters() const {return _parameters;}
    Base::DualNumber netError(const ValueSet& on) const;
    Base::DualNumber netSqError(const ValueSet& on) const;
    double netError() const;
    double weight() const {return _weight;}

    virtual HParaObject copy() const override;


public: //friends
    friend class ConstraintPy;
};

} //namespace

#endif
