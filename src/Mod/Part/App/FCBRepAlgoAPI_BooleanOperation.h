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

#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <Message_ProgressRange.hxx>

class FCBRepAlgoAPIHelper
{
public:
    static void setAutoFuzzy(BRepAlgoAPI_BooleanOperation* op);
    static void setAutoFuzzy(BRepAlgoAPI_BuilderAlgo* op);
};

class FCBRepAlgoAPI_BooleanOperation: public BRepAlgoAPI_BooleanOperation
{
public:
    DEFINE_STANDARD_ALLOC

    //! Empty constructor
    Standard_EXPORT FCBRepAlgoAPI_BooleanOperation();

    // set fuzzyness based on size
    void setAutoFuzzy();

    // not an override - real Build() has optionals, sadly type of those optionals that are differs
    // between OCCT versions
    Standard_EXPORT virtual void Build();  // NOLINT(clang-diagnostic-overloaded-virtual,
                                           // -Woverloaded-virtual)

#if OCC_VERSION_HEX >= 0x070600
    Standard_EXPORT void Build(const Message_ProgressRange& progressRange) Standard_OVERRIDE;
#else
    Standard_EXPORT virtual void Build(const Message_ProgressRange& progressRange);
#endif

protected:  //! @name Constructors
    //! Constructor to perform Boolean operation on only two arguments.
    //! Obsolete
    Standard_EXPORT FCBRepAlgoAPI_BooleanOperation(
        const TopoDS_Shape& theS1,
        const TopoDS_Shape& theS2,
        BOPAlgo_Operation theOperation
    );

private:
    Standard_EXPORT void RecursiveAddTools(const TopoDS_Shape& theTool);
    Standard_EXPORT void RecursiveCutFusedTools(
        const TopTools_ListOfShape& theOriginalArguments,
        const TopoDS_Shape& theTool
    );
    Standard_EXPORT void RecursiveCutCompound(const TopoDS_Shape& theArgument);
};
