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
#include "Position.h"
#include "ParaShape.h"
#include "ParaPoint.h"

namespace FCS {
namespace G2D {

class ParaCurve;
typedef Base::UnsafePyHandle<ParaCurve> HParaCurve;
typedef Base::UnsafePyHandle<ParaShape<ParaCurve>> HShape_Curve;

class FCSExport ParaCurve : public FCS::G2D::ParaGeometry2D
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
protected://data
    ///true if it's a full circle, not arc of circle. Not all curves support it.
    bool _isFull = false;

public://data
    ///starting point parameter
    ParameterRef u0;
    ///ending point parameter
    ParameterRef u1;
    ///starting point, may be None
    HParaPoint p0;
    ///ending point, may be None
    HParaPoint p1;

public://methods
    ParaCurve();
    void initAttrs() override;
    virtual PyObject* getPyObject() override;

    bool isFull() const {return _isFull;}

    ///returns curve point at parameter u
    virtual Position value(const ValueSet& vals, DualNumber u) = 0;
    ///returns tangent at parameter u
    virtual Vector tangent(const ValueSet& vals, DualNumber u) = 0;
    ///returns tangent given a point on or near the edge
    virtual Vector tangentAtXY(const ValueSet& vals, Position p);
    virtual bool supports_tangentAtXY(){return false;}
    ///returns n'th derivative by u
    virtual Vector D(const ValueSet& vals, DualNumber u, int n);
    virtual bool supports_D(){return false;}

    ///length of curve between specified parameters
    virtual DualNumber length(const ValueSet& vals, DualNumber u0, DualNumber u1);
    virtual DualNumber length(const ValueSet& vals);
    virtual bool supports_length() {return false;}

    ///length of untrimmed curve
    virtual DualNumber fullLength(const ValueSet& vals);
    virtual bool supports_fullLength() {return false;}

    ///error function for point-on-curve constraint
    virtual DualNumber pointOnCurveErrFunc(const ValueSet& vals, Position p);
    virtual bool supports_pointOnCurveErrFunc() {return false;}

private: //methods
    [[noreturn]] void throwFunctionNotSupported(std::string funcname) const;

public: //friends
    friend class ParaCurvePy;

};

}} //namespace

#endif
