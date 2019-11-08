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
#define GCSExport
#ifndef FREECAD_CONSTRAINTSOLVER_PARAMETER_H
#define FREECAD_CONSTRAINTSOLVER_PARAMETER_H

#include <string>

namespace GCS {

class ParameterRef;

class GCSExport Parameter
{
public://data
    double savedValue = 0.0;
    double scale = 1.0;
    double lowerlim = -1e100;
    double upperlim = 1e100;
    ///flag for implicit declaring of unknowns / data storage. Solvers can be instructed to vary any parameter regardless.
    bool fixed = false;
    int tag = 0;
    std::string label;
protected:
    int _ownIndex = -1;
    int _masterIndex = -1;
public://methods
    Parameter(){}
    /**
     * constructors are only for providing initializations for ParameterStore::add, like so:
     * store.add(Parameter("X", -5, 10))
     * Only parameters in a ParameterStore can be used for solver and geometry.
     */
    Parameter(double value, double scale = 1.0, bool fixed = false, int tag = 0);
    Parameter(const std::string& label, double value, double scale = 1.0, bool fixed = false, int tag = 0);
    ///returns index of the parameter in its container
    inline int ownIndex(){return _ownIndex;}
    inline int masterIndex(){return _masterIndex;}
    ///copy everything but indexes, from another parameter
    void pasteFrom(const Parameter& from);
    ///copy everything but indexes, from another parameter
    void pasteFrom(const ParameterRef from);
public://friends
    friend class ParameterStore;
};

} //namespace

#endif
