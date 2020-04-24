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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTHORIZONTAL_H
#define FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTHORIZONTAL_H

#include "SimpleConstraint.h"
#include "G2D/ParaPoint.h"

namespace FCS {
namespace G2D {

class ConstraintHorizontal;
typedef Base::UnsafePyHandle<ConstraintHorizontal> HConstraintHorizontal;

class FCSExport ConstraintHorizontal : public FCS::SimpleConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //data
    HShape_Point p1;
    HShape_Point p2;

public: //methods
    ConstraintHorizontal();

    void initAttrs() override;
    Base::DualNumber error1(const ValueSet& vals) const override;
    void setWeight(double weight) override;
    virtual PyObject* getPyObject() override;

public: //friends
    friend class ConstraintHorizontalPy;
};

}} //namespace

#endif
