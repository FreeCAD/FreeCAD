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

#ifndef FREECAD_CONSTRAINTSOLVER_DOGLEG_H
#define FREECAD_CONSTRAINTSOLVER_DOGLEG_H

#include "SolverBackend.h"

namespace FCS {

class Dogleg;
typedef UnsafePyHandle<Dogleg> HDogleg;

class FCSExport DogLeg : public SolverBackend
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    struct Prefs : public SolverBackend::Prefs
    {
    public://data
        double minGrad = 1e-80;
        double minStep = 1e-80;
        enum class eDogLegGaussStep : int {
            FullPivLU = 0,
            LeastNormFullPivLU = 1,
            LeastNormLdlt = 2
        };
        eDogLegGaussStep dogLegGaussStep = eDogLegGaussStep::FullPivLU;
        double initialTrustRegion = 0.1; //in parameter space
        double trustRegionExpandLinearityTolerance = 0.2; // how far is linearity factor allowed to depart from 1.0, to enable trust region expansion
        double trustRegionExpandMinStepSpan = 0.3; //if step doesn't span this much of trust region, don't expand trust region even if linearity is great
        double trustRegionExpandFactor = 3.0; //expand trust region by this factor
        double trustRegionShrinkLinearityThreshold = 0.25; //if linearity is worse than this (i.e., error is reducind this much slower than expected), shrink trust region.
        double trustRegionShrinkFactor = 2.0; //reduce trust region by this factor
        double trustRegionShrinkSpeedupFactor = 2.0; //multiply the reduction factor by this value if trust region is repeatedly reduced
    public://methods
        Py::Dict getPyValue() const override;
        void setAttr(std::string attrname, Py::Object value) override;
    };

protected: //data
    Prefs _prefs;
public:
    SolverBackend::Prefs& prefs() override{return _prefs;}

    eSolveResult solve(HSubSystem sys, HValueSet vals) override;
    eSolveResult solvePair(HSubSystem mainsys, HSubSystem auxsys, HValueSet vals) override;
};

} //namespace


#endif
