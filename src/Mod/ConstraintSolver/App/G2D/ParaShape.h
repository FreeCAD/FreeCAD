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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_ParaShapeBase_H
#define FREECAD_CONSTRAINTSOLVER_G2D_ParaShapeBase_H

#include <Mod/ConstraintSolver/App/ParaObject.h>
#include <Mod/ConstraintSolver/App/ParaGeometry.h>
#include "ParaTransform.h"

namespace FCS {
namespace G2D {

class ParaShapeBase;
typedef Base::UnsafePyHandle<ParaShapeBase> HParaShapeBase;

class FCSExport ParaShapeBase : public FCS::ParaObject
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data
    HParaTransform placement;
    HParaObject _tshape;
    bool reversed = false;

public://methods
    ParaShapeBase();
    ParaShapeBase(HParaGeometry tshape, HParaTransform placement = HParaTransform(Py::None()));
    void initAttrs() override;
    virtual PyObject* getPyObject() override;
    virtual std::string repr() const override;
    virtual HParaObject copy() const override;

    ///slaps transform of this shape atop another tshape. Useful for getting
    ///subshapes of this shape with placement information.
    HParaShapeBase getSubShape(const ParaGeometry& subshape);
    HParaShapeBase getSubShape(const HParaShapeBase subshape);

    Base::Type shapeType() const override {return _tshape->getTypeId();}

public: //friends
    friend class ParaShapePy;
};

template<class TShapeType>
class ParaShape : public ParaShapeBase
{
public:
    ParaShape();
    ParaShape(HParaTransform placement, UnsafePyHandle<TShapeType> tshape){
        this->placement = placement;
        this->_tshape = tshape;
    }
    TShapeType& tshape() {return static_cast<TShapeType&>(*_tshape);}
};

}} //namespace

#endif
