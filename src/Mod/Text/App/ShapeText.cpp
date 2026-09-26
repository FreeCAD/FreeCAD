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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDateTime>
#include <cmath>

#include <Font_BRepTextBuilder.hxx>
#endif

#include <App/FeaturePythonPyImp.h>
#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "ShapeText.h"
#include "ShapeTextPy.h"


using namespace Text;
using namespace Base;


FC_LOG_LEVEL_INIT("Text", true, true)

PROPERTY_SOURCE(Text::ShapeText, Part::Part2DObject)


const char* ShapeText::FontSourceEnums[] = {"Name", "Path", "Object", nullptr};

const char* ShapeText::AspectEnums[] = {"Normal", "Bold", "Italic", "BoldItalic", nullptr};

const char* ShapeText::JustificationEnums[] = {"TopLeft", "TopCenter", "TopRight",
                                 "MiddleLeft", "MiddleCenter", "MiddleRight",
                                 "BottomLeft", "BottomCenter", "BottomRight", nullptr};

const char* ShapeText::HeightReferenceEnums[] = {"CapHeight", "ShapeHeight", nullptr};

const char* ShapeText::DirectionEnums[] = {"LeftToRight", "RightToLeft", "TopToBottom", "BottomToTop", nullptr};


ShapeText::ShapeText()
    : fontInit(false)
#if OCC_VERSION_HEX >= 0x070500
    , myFormatter(new Font_TextFormatter)
#else
    , myFormatter()
#endif
{
    ADD_PROPERTY_TYPE(String, (""), "ShapeText", App::Prop_None, "Text string");
    ADD_PROPERTY_TYPE(FontSource, (false), "ShapeText", App::Prop_None, "Font comes from a family name, a file or a document object");
    FontSource.setEnums(FontSourceEnums);
    ADD_PROPERTY_TYPE(FontName, (""), "ShapeText", App::Prop_None, "Font family name");
    ADD_PROPERTY_TYPE(FontFile, (""), "ShapeText", App::Prop_None, "Font file path");
    ADD_PROPERTY_TYPE(FontObject, (nullptr), "ShapeText", App::Prop_None, "Font document object");
    ADD_PROPERTY_TYPE(Size, (5.0), "ShapeText", App::Prop_None, "Text height");
    ADD_PROPERTY_TYPE(Aspect, (0L), "ShapeText", App::Prop_None, "Font face aspect");
    Aspect.setEnums(AspectEnums);
    ADD_PROPERTY_TYPE(Justification, (6L), "ShapeText", App::Prop_None, "Horizontal and vertical alignment");
    Justification.setEnums(JustificationEnums);
    ADD_PROPERTY_TYPE(HeightReference, (0L), "ShapeText", App::Prop_None, "Height reference");
    HeightReference.setEnums(HeightReferenceEnums);
    ADD_PROPERTY_TYPE(Direction, (0L), "ShapeText", App::Prop_None, "Text direction");
    Direction.setEnums(DirectionEnums);
    ADD_PROPERTY_TYPE(KeepLeftMargin, (false), "ShapeText", App::Prop_None, "Keep left margin");
    ADD_PROPERTY_TYPE(ScaleToSize, (false), "ShapeText", App::Prop_None, "Scale to size");
}

ShapeText::~ShapeText() {}

PyObject* ShapeText::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new ShapeTextPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

bool ShapeText::initFont()
{
    switch (FontSource.getValue())
    {
        case 0:
            return myFont.FindAndInit(FontName.getValue(), getAspect(), Size.getValue());
            break;
        case 1:
            return myFont.Init(FontFile.getValue(), getAspect(), Size.getValue());
            break;
        default:
            Base::Console().error("Unknown font source value");
            return false;
    }
}

Font_FontAspect ShapeText::getAspect() const
{
    switch (Aspect.getValue())
    {
        case 0:
            return Font_FA_Regular;
        case 1:
            return Font_FA_Bold;
        case 2:
            return Font_FA_Italic;
        case 3:
            return Font_FA_BoldItalic;
        default:
            Base::Console().error("Unknown aspect value");
            return Font_FA_Regular;
    }
}

App::DocumentObjectExecReturn* ShapeText::execute()
{
    Base::Quantity size = Size.getQuantityValue();
    if (size <= Base::Quantity(Precision::Confusion(), Base::Unit::Length)) {
        Base::Console().error("Text size must not be zero");
        return Part2DObject::execute();
    }

    if (String.getMemSize() == 0)
        return Part2DObject::execute();

    if (!fontInit && !initFont())
        return Part2DObject::execute();

    fontInit = true;

    Graphic3d_HorizontalTextAlignment hAlign;
    Graphic3d_VerticalTextAlignment vAlign;
    switch (Justification.getValue())
    {
        case 0:
            hAlign = Graphic3d_HTA_LEFT;
            vAlign = Graphic3d_VTA_TOP;
            break;
        case 1:
            hAlign = Graphic3d_HTA_CENTER;
            vAlign = Graphic3d_VTA_TOP;
            break;
        case 2:
            hAlign = Graphic3d_HTA_RIGHT;
            vAlign = Graphic3d_VTA_TOP;
            break;
        case 3:
            hAlign = Graphic3d_HTA_LEFT;
            vAlign = Graphic3d_VTA_CENTER;
            break;
        case 4:
            hAlign = Graphic3d_HTA_CENTER;
            vAlign = Graphic3d_VTA_CENTER;
            break;
        case 5:
            hAlign = Graphic3d_HTA_RIGHT;
            vAlign = Graphic3d_VTA_CENTER;
            break;
        case 6:
            hAlign = Graphic3d_HTA_LEFT;
            vAlign = Graphic3d_VTA_BOTTOM;
            break;
        case 7:
            hAlign = Graphic3d_HTA_CENTER;
            vAlign = Graphic3d_VTA_BOTTOM;
            break;
        case 8:
            hAlign = Graphic3d_HTA_RIGHT;
            vAlign = Graphic3d_VTA_BOTTOM;
            break;
        default:
            Base::Console().error("Unknown justification value");
            hAlign = Graphic3d_HTA_LEFT;
            vAlign = Graphic3d_VTA_BOTTOM;
            break;
    }

    gp_Ax3 pen;

    myFormatter->Reset();
    myFormatter->SetupAlignment(hAlign, vAlign);
#if OCC_VERSION_HEX >= 0x070500
    myFormatter->Append(String.getValue(), *myFont.FTFont());
#else
    myFormatter->Append(String.getValue(), myFont);
#endif
    myFormatter->Format();

    Font_BRepTextBuilder textBuilder;
    Font_Rect bndBox;

#if OCC_VERSION_HEX >= 0x070500
    TopoDS_Shape shape(textBuilder.Perform(myFont, myFormatter, pen));
#else
    TopoDS_Shape shape(textBuilder.Perform(myFont, *myFormatter, pen));
#endif

    // if (HeightReference.getValue() == 1) {
        // myFormatter->BndBox(bndBox);
        // pen.SetCoord(0.0, bndBox.Height() / 2.0 - bndBox.Top, 0.0);
    // }

    Shape.setValue(shape);

    return Part2DObject::execute();
}

unsigned int ShapeText::getMemSize() const
{
    return 0;
}

void ShapeText::Save(Writer& writer) const
{
    Part::Part2DObject::Save(writer);
}

void ShapeText::Restore(XMLReader& reader)
{
    Part::Part2DObject::Restore(reader);
}

void ShapeText::onChanged(const App::Property* prop)
{
    if (prop == &FontSource) {
        FontName.setReadOnly(FontSource.getValue() == 0);
        FontFile.setReadOnly(FontSource.getValue() == 1);
        FontObject.setReadOnly(FontSource.getValue() == 2);
    }
    else if (prop == &FontName || prop == &FontFile || prop == &FontObject || prop == &Aspect) {
        fontInit = initFont();
    }
    else if (prop == &Size) {
        myFont.setSize(Size.getValue());
    }

    Part::Part2DObject::onChanged(prop);
}

// Python Text feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Text::ShapeTextPython, Text::ShapeText)
template<>
const char* Text::ShapeTextPython::getViewProviderName() const
{
    return "TextGui::ViewProviderShapeTextPython";
}
template<>
PyObject* Text::ShapeTextPython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<ShapeTextPy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class TextExport FeaturePythonT<Text::ShapeText>;
}// namespace App
