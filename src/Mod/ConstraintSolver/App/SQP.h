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

#ifndef FREECAD_CONSTRAINTSOLVER_SQP_H
#define FREECAD_CONSTRAINTSOLVER_SQP_H

#include "SolverBackend.h"

namespace FCS {

class SQP;
typedef UnsafePyHandle<SQP> HSQP;

class FCSExport SQP : public SolverBackend
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
     * @brief qp_eq minimizes ( 0.5 * x^T * H * x + g^T * x ) under the condition ( A*x + c = 0 ). A helper routine for solve
     * @param H:
     * @param g:
     * @param A: x_size by c_size matrix
     * @param c: vector of size c_size
     * @param x (output) - solution
     * @param Y (output) - row-space of A
     * @param Z (output) - null-space of A
     * @return it returns the solution in x, the row-space of A in Y, and the null space of A in Z
     */
    int qp_eq(const Eigen::MatrixXd& H, const Eigen::VectorXd& g, const Eigen::MatrixXd& A, const Eigen::VectorXd& c, Eigen::VectorXd& x, Eigen::MatrixXd& Y, Eigen::MatrixXd& Z);
};

} //namespace


#endif
