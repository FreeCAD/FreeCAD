// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Eric Price (CorvusCorax)                           *
 *                      <eric.price[at]tuebingen.mpg.de>                   *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

/**
 * FCBRepAlgoAPI provides a wrapper for various OCCT functions.
 */

#pragma once
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBuilderAPI_Command.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_BooleanOperation.h>


class FCBRepAlgoAPI_Section: public BRepAlgoAPI_Section
{
public:
    DEFINE_STANDARD_ALLOC


    //! Empty constructor
    Standard_EXPORT FCBRepAlgoAPI_Section();

    //! Constructor with two shapes
    //! <S1>  -argument
    //! <S2>  -tool
    //! <PerformNow> - the flag:
    //! if <PerformNow>=True - the algorithm is performed immediately
    //! Obsolete
    Standard_EXPORT FCBRepAlgoAPI_Section(
        const TopoDS_Shape& S1,
        const TopoDS_Shape& S2,
        const Standard_Boolean PerformNow = Standard_True
    );

    //! Constructor with two shapes
    //! <S1>  - argument
    //! <Pl>  - tool
    //! <PerformNow> - the flag:
    //! if <PerformNow>=True - the algorithm is performed immediately
    //! Obsolete
    Standard_EXPORT FCBRepAlgoAPI_Section(
        const TopoDS_Shape& S1,
        const gp_Pln& Pl,
        const Standard_Boolean PerformNow = Standard_True
    );

    // set fuzzyness based on size
    void setAutoFuzzy();
};
