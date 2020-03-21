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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTDIRECTIONALDISTANCE_H
#define FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTDIRECTIONALDISTANCE_H

#include "SimpleConstraint.h"
#include "ParaVector.h"
#include "ParaPoint.h"

namespace FCS {
namespace G2D {

class ConstraintDirectionalDistance;
typedef Base::UnsafePyHandle<ConstraintDirectionalDistance> HConstraintDirectionalDistance;

class FCSExport ConstraintDirectionalDistance : public FCS::SimpleConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //data
    HShape_Point p1;
    HShape_Point p2;
    ParameterRef dist;
protected: //data
    Vector _direction; //must be unit-length!

public: //methods
    ConstraintDirectionalDistance();
    ConstraintDirectionalDistance(HShape_Point p1, HShape_Point p2, Vector direction);
    static HConstraintDirectionalDistance makeConstraintHorizontalDistance(HShape_Point p1, HShape_Point p2);
    static HConstraintDirectionalDistance makeConstraintVerticalDistance(HShape_Point p1, HShape_Point p2);

    void initAttrs() override;
    Base::DualNumber error1(const ValueSet& vals) const override;
    virtual std::vector<ParameterRef> datumParameters() const override {return {dist};};
    virtual std::vector<Base::DualNumber> caluclateDatum(const ValueSet& vals) override;
    void setWeight(double weight) override;
    virtual void throwIfIncomplete() const override;
    virtual PyObject* getPyObject() override;
    HParaObject copy() const override;

    const Vector& direction() const {return _direction;}
    void setDirection(Vector newdirection);

public: //friends
    friend class ConstraintDirectionalDistancePy;
};

}} //namespace

#endif
