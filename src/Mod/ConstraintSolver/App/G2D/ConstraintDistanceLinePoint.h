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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTDISTANCELINEPOINT_H
#define FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTDISTANCELINEPOINT_H

#include "SimpleConstraint.h"
#include "ParaLine.h"

namespace FCS {
namespace G2D {

class ConstraintDistanceLinePoint;
typedef Base::UnsafePyHandle<ConstraintDistanceLinePoint> HConstraintDistanceLinePoint;

class FCSExport ConstraintDistanceLinePoint : public FCS::SimpleConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //data
    HShape_Line line;
    HShape_Point point;
    ParameterRef dist;

public: //methods
    ConstraintDistanceLinePoint();

    void initAttrs() override;
    Base::DualNumber error1(const ValueSet& vals) const override;
    virtual std::vector<ParameterRef> datumParameters() const override {return {dist};};
    virtual std::vector<Base::DualNumber> calculateDatum(const ValueSet& vals) override;
    void setWeight(double weight) override;

    virtual PyObject* getPyObject() override;

public: //helpers
    DualNumber calculateDistance(const ValueSet& vals) const;

public: //friends
    friend class ConstraintDistanceLinePointPy;
};

}} //namespace

#endif
