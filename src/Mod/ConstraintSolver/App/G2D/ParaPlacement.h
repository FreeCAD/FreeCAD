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

#ifndef FREECAD_CONSTRAINTSOLVER_PARAPLACEMENT_H
#define FREECAD_CONSTRAINTSOLVER_PARAPLACEMENT_H

#include <Mod/ConstraintSolver/App/ParaObject.h>
#include "ParaPoint.h"
#include "ParaVector.h"
#include "Placement.h"

namespace FCS {
namespace G2D {

class ParaPlacement;
typedef Base::UnsafePyHandle<ParaPlacement> HParaPlacement;

class FCSExport ParaPlacement : public FCS::ParaObject
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data
    HParaPoint translation;
    HParaVector rotation;

public://methods
    ParaPlacement();
    ParaPlacement(ParameterRef x, ParameterRef y, ParameterRef rx, ParameterRef ry);
    void initAttrs() override;
    virtual PyObject* getPyObject() override;

    Placement operator()(const ValueSet& vals) const;

public: //friends
    friend class ParaPlacementPy;

};

}} //namespace

#endif
