/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#ifndef TEXT_ShapeText_H
#define TEXT_ShapeText_H

#include <App/FeaturePython.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <CXX/Objects.hxx>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Text/TextGlobal.h>

#include "BRepFont.h"
#include <Font_TextFormatter.hxx>
#include <Standard_Version.hxx>

class Property;


namespace Text {

class TextExport ShapeText : public Part::Part2DObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Text::ShapeText);

public:
    ShapeText();
    ~ShapeText() override;

    App::PropertyString      String;
    App::PropertyEnumeration FontSource;
    App::PropertyString      FontName;
    App::PropertyFile        FontFile;
    App::PropertyLink        FontObject;
    App::PropertyLength      Size;
    App::PropertyEnumeration Aspect;
    App::PropertyEnumeration Justification;
    App::PropertyEnumeration HeightReference;
    App::PropertyEnumeration Direction;
    App::PropertyBool        KeepLeftMargin;
    App::PropertyBool        ScaleToSize;

    PyObject* getPyObject() override;

    bool initFont();

    Font_FontAspect getAspect() const;

    App::DocumentObjectExecReturn* execute() override;

    const char* getViewProviderName() const override
    {
        return "TextGui::ViewProviderShapeText";
    }

    unsigned int getMemSize() const override;

    void Save(Base::Writer& writer) const override;

    void Restore(Base::XMLReader& reader) override;

protected:
    void onChanged(const App::Property* prop) override;

private:
    static const char* FontSourceEnums[];
    static const char* AspectEnums[];
    static const char* JustificationEnums[];
    static const char* HeightReferenceEnums[];
    static const char* DirectionEnums[];

    bool fontInit;
    BRepFont myFont;
#if OCC_VERSION_HEX >= 0x070500
    Handle(Font_TextFormatter) myFormatter;
#else
    Font_TextFormatter myFormatter;
#endif
};

using ShapeTextPython = App::FeaturePythonT<ShapeText>;

} //namespace Text


#endif // TEXT_ShapeText_H
