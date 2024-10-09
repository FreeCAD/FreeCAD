/***************************************************************************
 *   Copyright (c) 2024 Eric Price (CorvusCorax) <eric.price@tuebingen.mpg.de>
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
/**
  * FCRepAlgoAPI provides a wrapper for various OCCT functions.
  */

#ifndef FCREPALGOAPISECTION_H
#define FCREPALGOAPISECTION_H
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBuilderAPI_Command.hxx>
#include <Mod/Part/App/FCRepAlgoAPI_BooleanOperation.h>


class FCRepAlgoAPI_Section : public BRepAlgoAPI_Section
{
public:

    DEFINE_STANDARD_ALLOC

  
    //! Empty constructor
    Standard_EXPORT FCRepAlgoAPI_Section();
  
    //! Constructor with two shapes
    //! <S1>  -argument
    //! <S2>  -tool
    //! <PerformNow> - the flag:
    //! if <PerformNow>=True - the algorithm is performed immediately
    //! Obsolete
    Standard_EXPORT FCRepAlgoAPI_Section(const TopoDS_Shape& S1, const TopoDS_Shape& S2, const Standard_Boolean PerformNow = Standard_True);

    //! Constructor with two shapes
    //! <S1>  - argument
    //! <Pl>  - tool
    //! <PerformNow> - the flag:
    //! if <PerformNow>=True - the algorithm is performed immediately
    //! Obsolete
    Standard_EXPORT FCRepAlgoAPI_Section(const TopoDS_Shape& S1, const gp_Pln& Pl, const Standard_Boolean PerformNow = Standard_True);


};
#endif
