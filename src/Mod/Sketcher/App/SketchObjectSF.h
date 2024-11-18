/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef SKETCH_SKETCHOBJECTSF_H
#define SKETCH_SKETCHOBJECTSF_H

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


#endif  // SKETCH_SKETCHOBJECTSF_H
