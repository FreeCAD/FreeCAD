/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef TECHDRAW_DrawTemplate_h_
#define TECHDRAW_DrawTemplate_h_

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyUnits.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw
{

class DrawPage;

class TechDrawExport DrawTemplate : public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawTemplate);

public:
    DrawTemplate(); /// Constructor
    ~DrawTemplate() override;

    // Page Physical Properties
    App::PropertyLength Width;
    App::PropertyLength Height;
    App::PropertyEnumeration Orientation;

    App::PropertyMap EditableTexts;

public:
    /// Returns template width in mm
    virtual double getWidth() const;
    /// Returns template height in mm
    virtual double getHeight() const;

    virtual DrawPage* getParentPage() const;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const override{
        return "TechDrawGui::ViewProviderTemplate";
    }

    // from base class
    PyObject *getPyObject(void) override;

private:
    static const char* OrientationEnums[];

};

using DrawTemplatePython = App::FeaturePythonT<DrawTemplate>;

} //namespace TechDraw



#endif //TECHDRAW_DrawTemplate_h_
