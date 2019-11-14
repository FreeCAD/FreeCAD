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

#ifndef FREECAD_CONSTRAINTSOLVER_SIMPLECONSTRAINT_H
#define FREECAD_CONSTRAINTSOLVER_SIMPLECONSTRAINT_H

#include "Constraint.h"

namespace FCS {

/**
 * @brief SimpleConstraint: a constraint returning a scalar error (removes 1 DOF). Override error1 instead of error.
 */
class FCSExport SimpleConstraint : public Constraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    int rank() const override {return 1;}
    void error(const ValueSet& vals, Base::DualNumber* returnbuf) const override;
    virtual Base::DualNumber error1(const ValueSet& vals) const = 0;
};

} //namespace

#endif
