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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTPLACEMENTRULES_H
#define FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTPLACEMENTRULES_H

#include "ParaGeometry.h"
#include "ParaPoint.h"
#include "SimpleConstraint.h"
#include "ParaPlacement.h"

namespace FCS {
namespace G2D {

class ConstraintPlacementRules;
typedef Base::UnsafePyHandle<ConstraintPlacementRules> HConstraintPlacementRules;

class FCSExport ConstraintPlacementRules : public FCS::SimpleConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //data
    HParaPlacement placement;

public: //methods
    ConstraintPlacementRules();
    ConstraintPlacementRules(HParaPlacement plm);

    void initAttrs() override;
    Base::DualNumber error1(const ValueSet& vals) const override;
    virtual PyObject* getPyObject() override;

public: //friends
    friend class ConstraintPlacementRulesPy;
};

}} //namespace

#endif
