/***************************************************************************
 *   Copyright (c) 2023 Andrea Reale <realeandrea@yahoo.it>                *
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
/** \file FCConsts.h
 *  \brief Freecad consts.
 */
 #ifndef FC_CONSTS_H
 #define FC_CONSTS_H
#include <limits>

    #ifndef DOUBLE_MAX
	#define DOUBLE_MAX 1.7976931348623157E+308 /* max decimal value of a "double"*/
    #endif
    #ifndef DOUBLE_MIN
        #define DOUBLE_MIN 2.2250738585072014E-308 /* min decimal value of a "double"*/
    #endif

#if CMAKE_CXX_STANDARD == 20
    #include <numbers>
#else
    #include <boost/math/constants/constants.hpp>
    static constexpr double pi_v = boost::math::constants::pi<double>();
    static constexpr double e_v  = boost::math::constants::e<double>();
    

     
#endif

//pi consts
static constexpr double pi_2v = pi_v * 2.0;
static constexpr double pi_3v = pi_v  * 3.0;
static constexpr double pi_4v = pi_v  * 4.0;
static constexpr double pi_1v_2 = pi_v  / 2.0;
static constexpr double pi_1v_3 = pi_v  / 3.0;
static constexpr double pi_1v_4 = pi_v  / 4.0;
static constexpr double pi_1v_6 = pi_v  / 6.0;
static constexpr double pi_1v_8 = pi_v  / 8.0;
static constexpr double pi_1v_12 = pi_v  / 12.0;
static constexpr double pi_1v_16 = pi_v  / 16.0;
static constexpr double pi_1v_18 = pi_v  / 18.0;
static constexpr double pi_2v_3 = pi_v  * 2.0/ 3.0;

#endif
 
