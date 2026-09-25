// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include "FeatureDressUp.h"

namespace PartDesign
{

class PartDesignExport Thickness: public DressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Thickness);

public:
    enum class SelectionMode
    {
        SelectedFaces = 0,
        SelectedSolids = 1,
        AllSolids = 2  // option only accessible via checkbox
    };
    // enum class ThicknessMode : int16_t {
    //     Skin = BRepOffset_Skin,
    //     Pipe = BRepOffset_Pipe,
    //     RectoVerso = BRepOffset_RectoVerso
    // };

    Thickness();

    App::PropertyLength Value;
    App::PropertyBool Reversed;
    App::PropertyBool Intersection;
    App::PropertyEnumeration Mode;
    App::PropertyEnumeration Join;
    App::PropertyEnumeration Selection;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderThickness";
    }
    void updatePreviewShape() override;
    //@}
private:
    struct ThicknessParameters
    {
        const TopoShape& input;
        TopoShape& result;
        const std::vector<std::string>& subStrings;

        /// Solid id = [ Faces ]
        /// std::vector is empty for solid selection
        std::map<int, std::vector<TopoShape>> selectedShapes;

        double thickness;
        double tolerance;
        bool intersection;
        int16_t mode;
        int join;
        int solidCount;
    };

    App::DocumentObjectExecReturn* identifySolids(ThicknessParameters& params);
    TopoShape makeSolidShell(const TopoShape& solid, const ThicknessParameters& params);
    App::DocumentObjectExecReturn* executeSelectedFaces(ThicknessParameters& params);
    App::DocumentObjectExecReturn* executeSelectedSolids(ThicknessParameters& params);
    App::DocumentObjectExecReturn* executeAllSolids(ThicknessParameters& params);
    void updatePreviewSelectedFaces(ThicknessParameters& params, std::vector<TopoShape>& previewShapes);
    void updatePreviewSelectedSolids(ThicknessParameters& params, std::vector<TopoShape>& previewShapes);
    void updatePreviewAllSolids(ThicknessParameters& params, std::vector<TopoShape>& previewShapes);
    TopoShape makeSolidPreview(const TopoShape& solid, const ThicknessParameters& params);

    static const char* ModeEnums[];
    static const char* JoinEnums[];
    static const char* SelectionEnums[];
};

}  // namespace PartDesign
