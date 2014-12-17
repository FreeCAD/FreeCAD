/***************************************************************************
 *   Copyright (c) Konstantinos Poulios      (logari81@gmail.com) 2011     *
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

#ifndef PLANEGCS_UTIL_H
#define PLANEGCS_UTIL_H

#include <vector>
#include <map>
#include <set>

namespace GCS
{
    typedef std::vector<double *> VEC_pD;
    typedef std::vector<double> VEC_D;
    typedef std::vector<int> VEC_I;
    typedef std::map<double *, double *> MAP_pD_pD;
    typedef std::map<double *, double> MAP_pD_D;
    typedef std::map<double *, int> MAP_pD_I;
    typedef std::set<double *> SET_pD;
    typedef std::set<int> SET_I;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

} //namespace GCS

#endif // PLANEGCS_UTIL_H
