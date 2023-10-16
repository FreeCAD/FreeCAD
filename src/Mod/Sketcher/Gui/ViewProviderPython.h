/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef SketcherGui_VIEWPROVIDERPYTHON_H
#define SketcherGui_VIEWPROVIDERPYTHON_H

#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Sketcher/Gui/ViewProviderSketch.h>


namespace SketcherGui
{

class SketcherGuiExport ViewProviderCustom: public ViewProviderSketch
{
    PROPERTY_HEADER_WITH_OVERRIDE(SketcherGui::ViewProviderCustom);

public:
    /// constructor
    ViewProviderCustom();
    /// destructor
    ~ViewProviderCustom() override;
    void updateData(const App::Property*) override;

protected:
    void onChanged(const App::Property* prop) override;
    std::map<const App::Property*, Gui::ViewProvider*> propView;
};

using ViewProviderPython = Gui::ViewProviderPythonFeatureT<ViewProviderSketch>;
using ViewProviderCustomPython = Gui::ViewProviderPythonFeatureT<ViewProviderCustom>;

}  // namespace SketcherGui


#endif  // SketcherGui_VIEWPROVIDERPYTHON_H
