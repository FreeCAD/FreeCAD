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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PARAEDGE_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PARAEDGE_H

#include "ParaGeometry2D.h"
#include "Point.h"
#include "ParaShape.h"

namespace FCS {
namespace G2D {

class ParaCurve;
typedef Base::UnsafePyHandle<ParaCurve> HParaCurve;
typedef Base::UnsafePyHandle<ParaShape<ParaCurve>> HShape_Point;

class FCSExport ParaCurve : public FCS::G2D::ParaGeometry2D
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data
    ///starting point parameter
    ParameterRef u0;
    ///ending point parameter
    ParameterRef u1;

public://methods
    void initAttrs() override;
    virtual PyObject* getPyObject() override;

    ///returns curve point at parameter u
    virtual Point value(const ValueSet& vals, DualNumber u) = 0;
    ///returns tangent at parameter u
    virtual Vector tangent(const ValueSet& vals, DualNumber u) = 0;
    ///returns tangent given a point on or near the edge
    virtual Vector tangentAtXY(const ValueSet& vals, Point p);
    virtual bool supports_tangentAtXY(){return false;}
    ///returns n'th derivative by u
    virtual Vector D(const ValueSet& vals, DualNumber u, int n);
    virtual bool supports_D(){return false;}

public: //friends
    friend class ParaCurvePy;

};

}} //namespace

#endif
