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

#ifndef FREECAD_CONSTRAINTSOLVER_BFGS_H
#define FREECAD_CONSTRAINTSOLVER_BFGS_H

#include "SolverBackend.h"

namespace FCS {

class BFGS;
typedef UnsafePyHandle<BFGS> HBFGS;

class FCSExport BFGS : public SolverBackend
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    struct Prefs : public SolverBackend::Prefs
    {
    public://data
        double minStep = 1e-10;
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

    /**
     * @brief lineSearch finds a minimum of error function along dir
     * @param vals: initial state of parameters, and the output. Must include
     * all parameters of the subsystem; can include more. Vals is actively
     * modified during the search, so one can't launch two in parallel on the
     * same copy of values.
     * @param dir: direction (indexes as of parameters of the subsystem)
     * @return what number was dir multiplied by to arrive to the minimum
     */
    double lineSearch(HSubSystem sys, ValueSet& vals, const Eigen::VectorXd& dir);
};

} //namespace


#endif
