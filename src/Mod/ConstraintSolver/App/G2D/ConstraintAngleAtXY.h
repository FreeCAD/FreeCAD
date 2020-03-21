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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTANGLEATXY_H
#define FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTANGLEATXY_H

#include "G2D/ConstraintAngle.h"

#include "G2D/ParaPoint.h"
#include "G2D/ParaCurve.h"

namespace FCS {
namespace G2D {

class ConstraintAngleAtXY;
typedef Base::UnsafePyHandle<ConstraintAngleAtXY> HConstraintAngleAtXY;

class FCSExport ConstraintAngleAtXY : public FCS::G2D::ConstraintAngle
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //data
    HShape_Point p;
    HShape_Curve crv1;
    HShape_Curve crv2;

public: //methods
    ConstraintAngleAtXY();

    void initAttrs() override;

    virtual PyObject* getPyObject() override;

public: //helpers
    ///computes true angle between tangent vectors of the curves
    Base::DualNumber calculateAngle(const ValueSet& vals) const override;

public: //friends
    friend class ConstraintAngleAtXYPy;
};

}} //namespace

#endif
