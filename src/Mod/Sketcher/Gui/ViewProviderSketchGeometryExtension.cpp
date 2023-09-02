/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include <Mod/Sketcher/Gui/ViewProviderSketchGeometryExtensionPy.h>

#include "ViewProviderSketchGeometryExtension.h"


using namespace SketcherGui;

//---------- Geometry Extension
TYPESYSTEM_SOURCE(SketcherGui::ViewProviderSketchGeometryExtension,
                  Part::GeometryPersistenceExtension)


ViewProviderSketchGeometryExtension::ViewProviderSketchGeometryExtension()
    : RepresentationFactor(1.0)
    , VisualLayerId(0)
{}

void ViewProviderSketchGeometryExtension::copyAttributes(Part::GeometryExtension* cpy) const
{
    Part::GeometryExtension::copyAttributes(cpy);
    static_cast<ViewProviderSketchGeometryExtension*>(cpy)->RepresentationFactor =
        this->RepresentationFactor;
    static_cast<ViewProviderSketchGeometryExtension*>(cpy)->VisualLayerId = this->VisualLayerId;
}

std::unique_ptr<Part::GeometryExtension> ViewProviderSketchGeometryExtension::copy() const
{
    auto cpy = std::make_unique<ViewProviderSketchGeometryExtension>();

    copyAttributes(cpy.get());

#if defined(__GNUC__) && (__GNUC__ <= 4)
    return std::move(cpy);
#else
    return cpy;
#endif
}

void ViewProviderSketchGeometryExtension::restoreAttributes(Base::XMLReader& reader)
{
    Part::GeometryPersistenceExtension::restoreAttributes(reader);

    if (reader.hasAttribute("visualLayerId")) {
        VisualLayerId = reader.getAttributeAsInteger("visualLayerId");
    }
}

void ViewProviderSketchGeometryExtension::saveAttributes(Base::Writer& writer) const
{
    Part::GeometryPersistenceExtension::saveAttributes(writer);

    writer.Stream() << "\" visualLayerId=\"" << VisualLayerId;
}


PyObject* ViewProviderSketchGeometryExtension::getPyObject()
{
    return new ViewProviderSketchGeometryExtensionPy(
        new ViewProviderSketchGeometryExtension(*this));
}
