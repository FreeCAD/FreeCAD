/***************************************************************************
 *   Copyright (c) 2016 York van Havre <yorik@uncreated.net>               *
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

#ifndef DrawViewArch_h_
#define DrawViewArch_h_

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <Base/BoundBox.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawViewSymbol.h"


namespace TechDraw
{

class TechDrawExport DrawViewArch : public TechDraw::DrawViewSymbol
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewArch);

public:
    /// Constructor
    DrawViewArch();
    ~DrawViewArch() override = default;

    App::PropertyLink         Source;
    App::PropertyBool         AllOn;
    App::PropertyEnumeration  RenderMode; // "Wireframe", "Solid"
    App::PropertyBool         FillSpaces;
    App::PropertyBool         ShowHidden;
    App::PropertyBool         ShowFill;
    App::PropertyFloat        LineWidth;
    App::PropertyFloat        FontSize;
    App::PropertyFloat        CutLineWidth;
    App::PropertyBool         JoinArch;
    App::PropertyFloat        LineSpacing;


    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderArch";
    }

    short mustExecute() const override;


protected:
/*    virtual void onChanged(const App::Property* prop) override;*/
    Base::BoundBox3d bbox;
    std::string getSVGHead();
    std::string getSVGTail();

private:
    static const char* RenderModeEnums[];
};

} //namespace TechDraw


#endif
