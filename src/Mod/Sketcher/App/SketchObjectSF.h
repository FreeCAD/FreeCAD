// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008 Jürgen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <App/PropertyFile.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Sketcher/SketcherGlobal.h>


namespace Sketcher
{

class SketchObjectSF: public Part::Part2DObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Sketcher::SketchObjectSF);

public:
    SketchObjectSF();

    /// Property
    App::PropertyFileIncluded SketchFlatFile;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// Uses the standard ViewProvider
    // const char* getViewProviderName(void) const {
    //     return "SketcherGui::ViewProviderSketchSF";
    // }
    //@}

    bool save(const char* FileName);
    bool load(const char* FileName);
};

}  // namespace Sketcher
