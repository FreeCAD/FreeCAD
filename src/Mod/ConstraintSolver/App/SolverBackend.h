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

#ifndef FREECAD_CONSTRAINTSOLVER_SOLVERBACKEND_H
#define FREECAD_CONSTRAINTSOLVER_SOLVERBACKEND_H

#include "SubSystem.h"

#include <Base/Console.h>

namespace FCS {

enum class eSolveResult : int{
    Success = 0,   // Found a solution zeroing the error function
    Minimized = 1, // Found a solution minimizing the error function
    Failed = 2,    // Failed to find any solution (typically, an error is thrown instead of returning this value)
};

enum class eDebugMode : int {
    NoDebug = 0,
    Minimal = 1,
    IterationLevel = 2
};

enum class ePairSolveMode : int {
    Sequential = 0,
    Merged = 1
};

class SolverBackend;
typedef UnsafePyHandle<SolverBackend> HSolverBackend;

class FCSExport SolverBackend : public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    ///preferences specialized for the actual algorithm
    struct FCSExport Prefs{
        public: //data
            ssize_t maxIter = 100; //max number of iterations
            bool maxIterSizeMult = false; //if true, multiply the maxIter by the number of parameters
            eDebugMode debugMode = eDebugMode::IterationLevel;
            double errorForSolved = 1e-10; //sets, how small the error is to become to declare as "solved"
            /**
             * @brief pairSolveMode sets, how to solve system pair with an
             * algorithm not natively supporting it.
             *
             * If Sequential, minimize merged system first, and then solve
             * mainsys. If Merged, minimize/solve merged system and just
             * return.
             */
            ePairSolveMode pairSolveMode = ePairSolveMode::Sequential;
        public: //methods
            virtual Py::Dict getPyValue() const = 0; //has default implementation, use it!
            virtual void setPyValue(Py::Dict d);
            virtual void setAttr(std::string attrname, Py::Object value) = 0; //has default implementation, use it!
            virtual ~Prefs() = default;

            ssize_t iterLimit(HSubSystem sys) const {return maxIterSizeMult ? maxIter * sys->params()->size() : maxIter;}
    };

protected://data
    PyObject* _twin = nullptr;

public: //methods

    virtual Prefs& prefs() = 0;

    /**
     * @brief solve solves a subsystem
     * @param sys: the subsystem
     * @param vals: the initial values and the output. Must include all parameters of the subsystem.
     * @return result code. If fails to find a solution, throws SolverError. Other errors may be thrown as well.
     */
    virtual eSolveResult solve(HSubSystem sys, HValueSet vals) = 0;

    ///solves a pair of systems, treating mainsys as "must" and auxsystem as "wanted but not required"
    virtual eSolveResult solvePair(HSubSystem mainsys, HSubSystem auxsys, HValueSet vals) = 0;

public://python
    virtual PyObject* getPyObject() override;
    
    template <  typename NewTypeT = SolverBackend,
                typename = typename std::enable_if<
                    std::is_base_of<SolverBackend, typename std::decay<NewTypeT>::type>::value
             >::type
    >
    UnsafePyHandle<NewTypeT> getHandle();
    

protected:
    ~SolverBackend() = default;
    friend class SolverBackendPy;

    ///logging shortcut
    template <typename... Args>
    inline void iterLog(std::string msg, Args... args){
        if(prefs().debugMode == eDebugMode::IterationLevel) {
            Base::Console().Log(msg.c_str(), args...);
            Base::Console().Log("\n");
        }
    }
};

template <  typename NewTypeT,
            typename 
>
UnsafePyHandle<NewTypeT> SolverBackend::getHandle() 
{
    return UnsafePyHandle<NewTypeT>(getPyObject(), true);
}


class FCSExport SolverError : public Base::Exception
{
public:
    /// Construction
    SolverError();
    SolverError(const char* sMessage);
    SolverError(const std::string& sMessage);
    /// Construction
    SolverError(const SolverError &inst);
    /// Destruction termination liquidation extermination
    virtual ~SolverError() throw() {}
    virtual PyObject* getPyExceptionType() const override;
};


} //namespace


#endif
