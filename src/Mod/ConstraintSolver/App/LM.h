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

#ifndef FREECAD_CONSTRAINTSOLVER_LM_H
#define FREECAD_CONSTRAINTSOLVER_LM_H

#include "SolverBackend.h"

namespace FCS {

class LM;
typedef UnsafePyHandle<LM> HLM;

class FCSExport LM : public SolverBackend
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    struct FCSExport Prefs : public SolverBackend::Prefs
    {
    public://data
        double minGrad = 1e-80;
        double initialDampingFactor = 1e-3;
        double dampingFactorBoostMultiplier = 2;
        double dampingFactorBoostSpeedupMultiplier = 2;
        double dampingFactorReductionMultiplier = 0.3333;
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
