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
#include "ParaLine.h"

namespace FCS {
namespace G2D {

class ParaConic;
typedef Base::UnsafePyHandle<ParaConic> HParaConic;
typedef Base::UnsafePyHandle<ParaShape<ParaConic>> HShape_Conic;

/**
 * @brief Base class for ellipse and hyperbola. Parabola is implemented
 * separately, and for being very different, does not benefit from deriving from
 * this class
 */
class FCSExport ParaConic : public FCS::G2D::ParaCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public://data

    HParaPoint center = nullptr;
    ParameterRef radmin;
    HParaPoint focus1 = nullptr;
    HParaPoint focus2 = nullptr;
    HParaLine minorDiameterLine = nullptr;
    HParaLine majorDiameterLine = nullptr;

public://methods

    virtual PyObject* getPyObject() override;

    ///returns full position of focus1 (focus1 attribute )
    virtual Position getFocus1(const ValueSet&) const = 0;
    Position getFocus2(const ValueSet&) const;
    virtual DualNumber getF(const ValueSet&) const = 0;
    virtual DualNumber getRMaj(const ValueSet&) const = 0;
    virtual DualNumber getRMin(const ValueSet&) const = 0;


public: //friends
    friend class ParaConicPy;

};

}} //namespace

#endif
