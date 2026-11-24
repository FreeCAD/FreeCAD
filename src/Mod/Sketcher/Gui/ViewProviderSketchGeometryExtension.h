// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2020 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/SketcherGlobal.h>


namespace SketcherGui
{

class SketcherGuiExport ViewProviderSketchGeometryExtension: public Part::GeometryPersistenceExtension
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
