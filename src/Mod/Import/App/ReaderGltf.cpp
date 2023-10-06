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
#if OCC_VERSION_HEX >= 0x070500
#include <BRep_Builder.hxx>
#include <Message_ProgressRange.hxx>
#include <Quantity_ColorRGBA.hxx>
#include <RWGltf_CafReader.hxx>
#include <TDF_Label.hxx>
#include <TDF_TagSource.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_VisMaterial.hxx>
#include <XCAFDoc_VisMaterialTool.hxx>
#endif
#endif

#include "ReaderGltf.h"
#include "Tools.h"
#include <Base/Exception.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Tools.h>

using namespace Import;

// NOLINTNEXTLINE
ReaderGltf::ReaderGltf(const Base::FileInfo& file)
    : file {file}
{}

// NOLINTNEXTLINE
void ReaderGltf::read(Handle(TDocStd_Document) hDoc)
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

    processDocument(hDoc);

#else
    (void)hDoc;
    throw Base::RuntimeError("gITF support requires OCCT 7.5.0 or later");
#endif
}

// NOLINTNEXTLINE
void ReaderGltf::processDocument(Handle(TDocStd_Document) hDoc)
{
#if OCC_VERSION_HEX >= 0x070500
    Handle(XCAFDoc_ShapeTool) aShapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());

    TDF_LabelSequence shapeLabels;
    aShapeTool->GetShapes(shapeLabels);
    for (Standard_Integer i = 1; i <= shapeLabels.Length(); i++) {
        auto topLevelshape = shapeLabels.Value(i);
        TopoDS_Shape shape = aShapeTool->GetShape(topLevelshape);
        if (!shape.IsNull()) {
            TDF_LabelSequence subShapeLabels;
            if (XCAFDoc_ShapeTool::GetSubShapes(topLevelshape, subShapeLabels)) {
                TopoDS_Shape compound = processSubShapes(hDoc, subShapeLabels);
                aShapeTool->SetShape(topLevelshape, compound);
            }
            else {
                aShapeTool->SetShape(topLevelshape, fixShape(shape));
            }
        }
    }
#endif
}

// NOLINTNEXTLINE
TopoDS_Shape ReaderGltf::processSubShapes(Handle(TDocStd_Document) hDoc,
                                          const TDF_LabelSequence& subShapeLabels)
{
    TopoDS_Compound compound;
#if OCC_VERSION_HEX >= 0x070500
    Handle(XCAFDoc_ShapeTool) aShapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
    Handle(XCAFDoc_ColorTool) aColorTool = XCAFDoc_DocumentTool::ColorTool(hDoc->Main());
    Handle(XCAFDoc_VisMaterialTool) aVisTool = XCAFDoc_DocumentTool::VisMaterialTool(hDoc->Main());

    BRep_Builder builder;
    builder.MakeCompound(compound);
    for (Standard_Integer i = 1; i <= subShapeLabels.Length(); i++) {
        auto faceLabel = subShapeLabels.Value(i);

        // OCCT handles colors of a glTF with material labels but the ImportOCAF(2) class
        // expects color labels. Thus, the material labels are converted into color labels.
        Handle(XCAFDoc_VisMaterial) aVisMat = aVisTool->GetShapeMaterial(faceLabel);

        Quantity_ColorRGBA rgba;
        bool hasVisMat {false};
        if (!aVisMat.IsNull()) {
            rgba = aVisMat->BaseColor();
            hasVisMat = true;
        }

        TopoDS_Shape face = aShapeTool->GetShape(faceLabel);
        TopoDS_Shape fixed = fixShape(face);
        builder.Add(compound, fixed);
        aShapeTool->SetShape(faceLabel, fixed);

        if (hasVisMat) {
            aColorTool->SetColor(faceLabel, rgba, XCAFDoc_ColorSurf);
        }
    }
#endif

    return {std::move(compound)};
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
