/***************************************************************************
 *   Copyright (c) 2020 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTANGLE_H
#define FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTANGLE_H

#include "SimpleConstraint.h"

namespace FCS {
namespace G2D {

class ConstraintAngle;
typedef Base::UnsafePyHandle<ConstraintAngle> HConstraintAngle;

class FCSExport ConstraintAngle : public FCS::SimpleConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //data
    ParameterRef angle;
    bool supplementAngle = false; //if set, true angle = 180deg - angle. This is to convert obtuse angles to have acute value.

public: //methods
    ConstraintAngle();

    void initAttrs() override;

    Base::DualNumber error1(const ValueSet& vals) const override;
    virtual std::vector<ParameterRef> datumParameters() const override {return {angle};};
    virtual std::vector<Base::DualNumber> calculateDatum(const ValueSet& vals) override;

    ///change supplementAngle and update angle value
    void convertToSupplement(HValueSet vals = nullptr);
    ///change Reversed and update angle value
    void convertToReversed(HValueSet vals = nullptr);

    virtual PyObject* getPyObject() override;
    virtual HParaObject copy() const override;

public: //helpers
    ///computes true angle between tangent vectors of the curves
    virtual Base::DualNumber calculateAngle(const ValueSet& vals) const = 0;

public: //friends
    friend class ConstraintAnglePy;
};

}} //namespace

#endif
