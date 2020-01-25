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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PARAPOINT_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PARAPOINT_H

#include "ParaGeometry2D.h"
#include "Position.h"
#include "ParaShape.h"

namespace FCS {
namespace G2D {

class ParaPoint;
typedef Base::UnsafePyHandle<ParaPoint> HParaPoint;
typedef Base::UnsafePyHandle<ParaShape<ParaPoint>> HShape_Point;

class FCSExport ParaPoint : public FCS::G2D::ParaGeometry2D
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data
    ParameterRef x;
    ParameterRef y;

public://methods
    ParaPoint();
    ParaPoint(ParameterRef x, ParameterRef y);
    void initAttrs() override;
    virtual PyObject* getPyObject() override;

    Position value(const ValueSet& vals) const;
    Position operator()(const ValueSet& vals) const{return value(vals);}

public: //friends
    friend class ParaPointPy;

};

}} //namespace

#endif
