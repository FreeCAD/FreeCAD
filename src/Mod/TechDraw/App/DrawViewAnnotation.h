/***************************************************************************
 *   Copyright (c) 2012 Yorik van Havre <yorik@uncreated.net>              *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyUnits.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawView.h"


namespace TechDraw
{

class TechDrawExport DrawViewAnnotation : public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewAnnotation);

public:
    /// Constructor
    DrawViewAnnotation();
    ~DrawViewAnnotation() override = default;

    App::PropertyStringList   Text;
    App::PropertyFont         Font;
    App::PropertyColor        TextColor;
    App::PropertyLength       TextSize;
    App::PropertyInteger      LineSpace;
    App::PropertyEnumeration  TextStyle; // Plain, Bold, Italic, Bold-Italic
    App::PropertyFloat        MaxWidth;
    App::PropertyLink         Owner;

    QRectF getRect() const override;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderAnnotation";
    }

    short mustExecute() const override;

    bool checkFit() const override {return true;}
    App::PropertyLink *getOwnerProperty() override { return &Owner; }

protected:
    void onChanged(const App::Property* prop) override;
    void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop) override;

private:
    static const char* TextStyleEnums[];
};

using DrawViewAnnotationPython = App::FeaturePythonT<DrawViewAnnotation>;


} //namespace TechDraw