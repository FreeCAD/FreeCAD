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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PARAGEOMETRY2D_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PARAGEOMETRY2D_H

#include <Mod/ConstraintSolver/App/ParaGeometry.h>
#include "ParaShape.h"

namespace FCS {
namespace G2D {

class ParaGeometry2D;

template <  typename TShape
>
auto toDShape(TShape t) -> decltype((new ParaShape<TShape>((new ParaTransform())->getHandle<ParaTransform>(), t. template getHandle<TShape>()))-> template getHandle<ParaShape<TShape>>()) {
    // to be improved with singleton identify transformation
    return (new ParaShape<TShape>((new ParaTransform())->getHandle<ParaTransform>(), t. template getHandle<TShape>()))-> template getHandle<ParaShape<TShape>>();
}
    
typedef UnsafePyHandle<ParaGeometry2D> HParaGeometry2D;

class FCSExport ParaGeometry2D : public FCS::ParaGeometry
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data

public://methods
    virtual PyObject* getPyObject() override;
    virtual HParaObject toShape() override;

public: //friends
    friend class ParaGeometry2DPy;

};




}} //namespace

#endif
