// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <Standard_Version.hxx>
#include <TDF_Label.hxx>
#include <TDF_TagSource.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#if OCC_VERSION_HEX >= 0x070500
#include <Message_ProgressRange.hxx>
#include <RWGltf_CafReader.hxx>
#endif
#endif

#include "ReaderGltf.h"
#include <Base/Exception.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Tools.h>

using namespace Import;

ReaderGltf::ReaderGltf(const Base::FileInfo& file)  // NOLINT
    : file {file}
{}

void ReaderGltf::read(Handle(TDocStd_Document) hDoc)  // NOLINT
{
#if OCC_VERSION_HEX >= 0x070500
    const double unit = 0.001;  // mm
    RWGltf_CafReader aReader;
    aReader.SetSystemLengthUnit(unit);
    aReader.SetSystemCoordinateSystem(RWMesh_CoordinateSystem_Zup);
    aReader.SetDocument(hDoc);
    aReader.SetParallel(true);

    TCollection_AsciiString filename(file.filePath().c_str());
    Standard_Boolean ret = aReader.Perform(filename, Message_ProgressRange());
    if (!ret) {
        throw Base::FileException("Cannot read from file: ", file);
    }

    Handle(XCAFDoc_ShapeTool) aShapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
    TDF_LabelSequence labels;
    aShapeTool->GetShapes(labels);
    for (Standard_Integer i = 1; i <= labels.Length(); i++) {
        auto label = labels.Value(i);
        TopoDS_Shape shape = aShapeTool->GetShape(label);
        if (!shape.IsNull()) {
            aShapeTool->SetShape(label, fixShape(shape));
        }
    }

#else
    (void)hDoc;
    throw Base::RuntimeError("gITF support requires OCCT 7.5.0 or later");
#endif
}

bool ReaderGltf::cleanup() const
{
    return clean;
}

void ReaderGltf::setCleanup(bool value)
{
    clean = value;
}

TopoDS_Shape ReaderGltf::fixShape(TopoDS_Shape shape)  // NOLINT
{
    // The glTF reader creates a compound of faces that only contains the triangulation
    // but not the underlying surfaces. This leads to faces without boundaries.
    // The triangulation is used to create a valid shape.
    const double tolerance = 0.5;
    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Facet> facets;
    Part::TopoShape sh(shape);
    sh.getFaces(points, facets, tolerance);
    sh.setFaces(points, facets, tolerance);

    if (cleanup()) {
        sh.sewShape();
        return sh.removeSplitter();
    }

    return sh.getShape();
}
