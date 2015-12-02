/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef PARTGUI_VIEWPROVIDERPART_H
#define PARTGUI_VIEWPROVIDERPART_H

#include <Standard_math.hxx>
#include <Standard_Boolean.hxx>
#include <TopoDS_Shape.hxx>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/ViewProviderBuilder.h>
#include <Mod/Part/Gui/ViewProviderExt.h>

class SoSeparator;

namespace Part { struct ShapeHistory; }

namespace PartGui {

class ViewProviderShapeBuilder : public Gui::ViewProviderBuilder
{
public:
    ViewProviderShapeBuilder(){}
    ~ViewProviderShapeBuilder(){}
    virtual void buildNodes(const App::Property*, std::vector<SoNode*>&) const;
    void createShape(const App::Property*, SoSeparator*) const;
};

class PartGuiExport ViewProviderPart : public ViewProviderPartExt
{
    PROPERTY_HEADER(PartGui::ViewProviderPart);

public:
    /// constructor
    ViewProviderPart();
    /// destructor
    virtual ~ViewProviderPart();
    virtual bool doubleClicked(void);

protected:
    void applyColor(const Part::ShapeHistory& hist,
                    const std::vector<App::Color>& colBase,
                    std::vector<App::Color>& colBool);
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERPART_H

