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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PARACONIC_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PARACONIC_H

#include "ParaCurve.h"
#include "ParaPoint.h"

namespace FCS {
namespace G2D {

class ParaConic;
typedef Base::UnsafePyHandle<ParaConic> HParaConic;
typedef Base::UnsafePyHandle<ParaShape<ParaConic>> HShape_Conic;

class FCSExport ParaConic : public FCS::G2D::ParaCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data

public://methods
    ParaConic();
    ParaConic(HParaPoint p1, HParaPoint p2);
    void initAttrs() override;
    virtual std::vector<ParameterRef> makeParameters(HParameterStore into) override;
    virtual PyObject* getPyObject() override;

    virtual Position value(const ValueSet& vals, DualNumber u) override;
    virtual Vector tangent(const ValueSet& vals, DualNumber u) override;
    virtual Vector tangentAtXY(const ValueSet& vals, Position p) override;
    virtual bool supports_tangentAtXY() override {return true;}

    virtual DualNumber length(const ValueSet& vals, DualNumber u0, DualNumber u1) override;
    virtual DualNumber length(const ValueSet& vals) override;
    virtual bool supports_length() override {return true;}

    virtual DualNumber pointOnCurveErrFunc(const ValueSet& vals, Position p) override;
    virtual bool supports_pointOnCurveErrFunc() override {return true;}

public: //friends
    friend class ParaConicPy;

};

}} //namespace

#endif
