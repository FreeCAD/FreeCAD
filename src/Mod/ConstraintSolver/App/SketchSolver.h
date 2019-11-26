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

#ifndef FREECAD_CONSTRAINTSOLVER_SKETCHSOLVER_H
#define FREECAD_CONSTRAINTSOLVER_SKETCHSOLVER_H

#include "SketchSolverSystem.h"

namespace FCS {

class SketchSolver;
typedef UnsafePyHandle<SketchSolver> HSketchSolver;

class SketchSolver
{
protected://structs and enums
    struct SystemPair{
        HSubSystem mainSys;
        HSubSystem auxSys;
    };
    struct DogLegPrefs{
        double tolg = 1e-80; //when error multiplied by transposed jacobian is smaller than this, iteration is aborted
        double tolx = 1e-80;
        double tolf = 1e-10; //when all error values <= this, consider solved and stop iteration
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
    };
    struct SolverPrefs{
        ssize_t maxIter = 100; //max number of iterations
        bool sizemult = false; //if true, multiply the maxIter by the number of parameters
        ssize_t iterLimit(HSubSystem sys) const {return sizemult ? maxIter * sys->params()->size() : maxIter;}

        enum class eDebugMode : int {
            NoDebug = 0,
            Minimal = 1,
            IterationLevel = 2
        };
        eDebugMode debugMode = eDebugMode::Minimal;
    };

    enum class eSolveResult : int{
        Success = 0,   // Found a solution zeroing the error function
        Minimized = 2, // Found a solution minimizing the error function
        Failed = 3,    // Failed to find any solution
    };

protected://data
    PyObject* _twin;

    HSubSystem _fullSystem;
    std::vector<SystemPair> _subsystems;

public://data
    SolverPrefs solverPrefs;

public://methods
    /**
     * @brief solveDogLeg: solve a subsystem with DogLeg solver.
     * @param sys: the subsystem
     * @param vals: the initial values and the output. Must include all parameters of the subsystem.
     * @param prefs: solver preferences
     * @return result code (success or failure). The solution or the point where it failed remains in vals.
     */
    eSolveResult solveDogLeg(HSubSystem sys, HValueSet vals, DogLegPrefs prefs);



    template <typename... Args>
    inline void iterLog(std::string msg, Args... args);

};

} //namespace


#endif
