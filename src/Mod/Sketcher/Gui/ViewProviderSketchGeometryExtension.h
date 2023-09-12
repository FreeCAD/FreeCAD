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


namespace SketcherGui
{

class SketcherGuiExport ViewProviderSketchGeometryExtension
    : public Part::GeometryPersistenceExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ViewProviderSketchGeometryExtension();
    ~ViewProviderSketchGeometryExtension() override = default;

    std::unique_ptr<Part::GeometryExtension> copy() const override;

    PyObject* getPyObject() override;

    // Data Members

    // Representation factor
    // Provides a mechanism to store a factor associated with the representation of a geometry
    // This is only useful when a geometry must be scaled only for representation, while keeping its
    // value Applicability: General abstract concepts embodied in a geometry, in practice B-Spline
    // poles. Why not in SketchGeometryExtension? Because it is merely representation related. It
    // has no place in a console application.
    double getRepresentationFactor() const
    {
        return RepresentationFactor;
    }
    void setRepresentationFactor(double representationFactor)
    {
        RepresentationFactor = representationFactor;
    }

    int getVisualLayerId() const
    {
        return VisualLayerId;
    }
    void setVisualLayerId(int visuallayerid)
    {
        VisualLayerId = visuallayerid;
    }

protected:
    void copyAttributes(Part::GeometryExtension* cpy) const override;
    void restoreAttributes(Base::XMLReader& reader) override;
    void saveAttributes(Base::Writer& writer) const override;

private:
    ViewProviderSketchGeometryExtension(const ViewProviderSketchGeometryExtension&) = default;

private:
    double RepresentationFactor;
    int VisualLayerId;
};

}  // namespace SketcherGui


#endif  // SKETCHER_VIEWPROVIDERSKETCHGEOMETRYEXTENSION_H
