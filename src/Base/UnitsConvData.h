/************************************************************************
 *                                                                      *
 *   This file is part of the FreeCAD CAx development system.           *
 *                                                                      *
 *   This library is free software; you can redistribute it and/or      *
 *   modify it under the terms of the GNU Library General Public        *
 *   License as published by the Free Software Foundation; either       *
 *   version 2 of the License, or (at your option) any later version.   *
 *                                                                      *
 *   This library  is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.               *
 *                                                                      *
 *   You should have received a copy of the GNU Library General Public  *
 *   License along with this library; see the file COPYING.LIB. If not, *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,      *
 *   Suite 330, Boston, MA  02111-1307, USA                             *
 *                                                                      *
 ************************************************************************/

#ifndef BASE_UNITSCONVDATA_H
#define BASE_UNITSCONVDATA_H

namespace Base::UnitsConvData
{

constexpr auto in {25.4};
constexpr auto ft {12 * in};
constexpr auto yd {3 * ft};
constexpr auto mi {1760 * yd};
constexpr auto lb {0.45359237};
constexpr auto lbf {9.80665 * lb};
constexpr auto psi {lbf / (in * in) * 1000};

}  // namespace Base::UnitsConvData

#endif
