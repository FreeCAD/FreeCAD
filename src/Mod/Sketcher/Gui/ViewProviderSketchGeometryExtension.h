/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHER_VIEWPROVIDERSKETCHGEOMETRYEXTENSION_H
#define SKETCHER_VIEWPROVIDERSKETCHGEOMETRYEXTENSION_H

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/SketcherGlobal.h>

namespace SketcherGui {

class SketcherGuiExport ViewProviderSketchGeometryExtension : public Part::GeometryExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:

    ViewProviderSketchGeometryExtension();
    virtual ~ViewProviderSketchGeometryExtension() override = default;

    virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

    virtual PyObject *getPyObject(void) override;

    // Data Members

    // Representation factor
    // Provides a mechanism to store a factor associated with the representation of a geometry
    // This is only useful when a geometry must be scaled only for representation, while keeping its value
    // Applicability: General abstract concepts embodied in a geometry, in practice B-Spline poles.
    // Why not in SketchGeometryExtension? Because it is merely representation related. It has no place in
    // a console application.
    virtual double getRepresentationFactor() const {return RepresentationFactor;}
    virtual void setRepresentationFactor(double representationFactor) {RepresentationFactor = representationFactor;}

protected:
    virtual void copyAttributes(Part::GeometryExtension * cpy) const override;

private:
    ViewProviderSketchGeometryExtension(const ViewProviderSketchGeometryExtension&) = default;

private:
    double RepresentationFactor;
};

} //namespace SketcherGui


#endif // SKETCHER_VIEWPROVIDERSKETCHGEOMETRYEXTENSION_H
