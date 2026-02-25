// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#pragma once

#include <Mod/Part/PartGlobal.h>
#include <Standard_CString.hxx>


namespace Part
{

class PartExport Interface
{
public:
    // See https://dev.opencascade.org/doc/overview/html/occt_user_guides__step.html
    enum class Assembly
    {
        Off = 0,  /**< (Default) writes STEP files without assemblies */
        On = 1,   /**< writes all shapes in the form of STEP assemblies */
        Auto = 2, /**< writes shapes having a structure of (possibly nested)
                       TopoDS_Compounds in the form of STEP assemblies, single
                       shapes are written without assembly structures */
    };
    enum class Unit
    {
        Millimeter = 0,
        Meter = 1,
        Inch = 2,
    };

    /** STEP settings */
    //@{
    static void writeStepAssembly(Assembly);
    static Standard_CString writeStepScheme();
    static bool writeStepScheme(Standard_CString);
    static Standard_CString writeStepUnit();
    static bool writeStepUnit(Standard_CString);
    static bool writeStepUnit(Unit);
    static Standard_CString writeStepHeaderProduct();
    static bool writeStepHeaderProduct(Standard_CString);
    //@}

    /** IGES settings */
    //@{
    static Standard_CString writeIgesHeaderAuthor();
    static bool writeIgesHeaderAuthor(Standard_CString);
    static Standard_CString writeIgesHeaderCompany();
    static bool writeIgesHeaderCompany(Standard_CString);
    static Standard_CString writeIgesHeaderProduct();
    static bool writeIgesHeaderProduct(Standard_CString);
    static Standard_CString writeIgesUnit();
    static bool writeIgesUnit(Standard_CString);
    static bool writeIgesUnit(Unit);
    static int writeIgesBrepMode();
    static bool writeIgesBrepMode(int);
    //@}
};

}  // namespace Part
