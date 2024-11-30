/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTDESIGNGUI_ENUMFLAGS_H
#define PARTDESIGNGUI_ENUMFLAGS_H

namespace PartDesignGui {

// https://wiggling-bits.net/using-enum-classes-as-type-safe-bitmasks/
// https://www.boost.org/doc/libs/1_66_0/boost/detail/bitmask.hpp
// https://stackoverflow.com/questions/1448396/how-to-use-enums-as-flags-in-c

enum class AllowSelection {
    NONE           = 0,      /**< This is used to indicate to stop the selection */
    EDGE           = 1 << 0, /**< Allow picking edges */
    FACE           = 1 << 1, /**< Allow picking faces */
    PLANAR         = 1 << 2, /**< Allow only linear edges and planar faces */
    CIRCLE         = 1 << 3, /**< Allow picking circular edges (incl arcs) */
    POINT          = 1 << 4, /**< Allow picking datum points */
    OTHERBODY      = 1 << 5, /**< Allow picking objects from another body in the same part */
    WHOLE          = 1 << 6  /**< Allow whole object selection */
};
Q_DECLARE_FLAGS(AllowSelectionFlags, AllowSelection)

} //namespace PartDesignGui

Q_DECLARE_OPERATORS_FOR_FLAGS(PartDesignGui::AllowSelectionFlags)

#endif // PARTDESIGNGUI_ENUMFLAGS_H
