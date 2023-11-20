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
#include <gp_Trsf.hxx>

#include "TKJtReader.h"
#include <Base/Console.h>
#include <Base/FileInfo.h>

using namespace JtReaderNS;

namespace
{
// clang-format off
const Handle(Standard_Type) Type_JtAttribute_GeometricTransform = STANDARD_TYPE(JtAttribute_GeometricTransform);
const Handle(Standard_Type) Type_JtAttribute_Material           = STANDARD_TYPE(JtAttribute_Material);
const Handle(Standard_Type) Type_JtNode_Partition               = STANDARD_TYPE(JtNode_Partition);
const Handle(Standard_Type) Type_JtNode_Group                   = STANDARD_TYPE(JtNode_Group);
const Handle(Standard_Type) Type_JtNode_Instance                = STANDARD_TYPE(JtNode_Instance);
const Handle(Standard_Type) Type_JtNode_RangeLOD                = STANDARD_TYPE(JtNode_RangeLOD);
const Handle(Standard_Type) Type_JtNode_Shape_Base              = STANDARD_TYPE(JtNode_Shape_Base);
const Handle(Standard_Type) Type_JtNode_Shape_Vertex            = STANDARD_TYPE(JtNode_Shape_Vertex);
const Handle(Standard_Type) Type_JtNode_Shape_TriStripSet       = STANDARD_TYPE(JtNode_Shape_TriStripSet);
// clang-format on
}  // namespace

TKJtReader::TKJtReader()
    : builder {result}
{}

void TKJtReader::clear()
{
    result.str(std::string());
    result.clear();
}

void TKJtReader::open(const std::string& filename)
{
    clear();

    Base::FileInfo file(filename);
    jtDir = TCollection_ExtendedString(TCollection_AsciiString((file.dirPath() + '/').c_str()));

    TCollection_ExtendedString aFileName(filename.c_str());
    Handle(JtData_Model) aModel = new JtData_Model(aFileName);

    if (!aModel.IsNull()) {
        Handle(JtNode_Partition) aNode = aModel->Init();
        if (!aNode.IsNull()) {
            rootNode = aNode;

            builder.addHeader();
            builder.beginSeparator();
            Base::ShapeHintsItem shapeHints;
            shapeHints.setShapeType(Base::ShapeType::Type::UnknownShapeType);
            shapeHints.setVertexOrdering(Base::VertexOrdering::Ordering::CounterClockwise);
            builder.addNode(shapeHints);
            traverseGraph(aNode);
            builder.endSeparator();
            rootNode.Nullify();
        }
    }
}

std::string TKJtReader::getOutput() const
{
    return result.str();
}

void TKJtReader::readMaterialAttribute(const Handle(JtAttribute_Material) & aMaterial)
{
    const Jt_F32* ambient = aMaterial->AmbientColor();
    const Jt_F32* diffuse = aMaterial->DiffuseColor();
    const Jt_F32* specular = aMaterial->SpecularColor();
    // const Jt_F32* emissive = aMaterial->EmissionColor();
    Jt_F32 shininess = aMaterial->Shininess() / 100.0F;

    // NOLINTBEGIN
    Base::MaterialItem item;
    item.setAmbientColor({Base::ColorRGB(ambient[0], ambient[1], ambient[2])});
    item.setDiffuseColor({Base::ColorRGB(diffuse[0], diffuse[1], diffuse[2])});
    item.setSpecularColor({Base::ColorRGB(specular[0], specular[1], specular[2])});
    // Ignore the emissive color as there are sometimes some weird values
    // item.setEmissiveColor({Base::ColorRGB(emissive[0], emissive[1], emissive[2])});
    item.setShininess({shininess});
    // NOLINTEND

    builder.addNode(item);
}

void TKJtReader::readTransformAttribute(const Handle(JtAttribute_GeometricTransform) & aTransform)
{
    gp_Trsf trsf;
    aTransform->GetTrsf(trsf);
    gp_Mat mat = trsf.VectorialPart();
    gp_XYZ xyz = trsf.TranslationPart();
    Base::Matrix4D mtrx;
    // NOLINTBEGIN
    mtrx[0][0] = mat(1, 1);
    mtrx[0][1] = mat(1, 2);
    mtrx[0][2] = mat(1, 3);

    mtrx[1][0] = mat(2, 1);
    mtrx[1][1] = mat(2, 2);
    mtrx[1][2] = mat(2, 3);

    mtrx[2][0] = mat(3, 1);
    mtrx[2][1] = mat(3, 2);
    mtrx[2][2] = mat(3, 3);

    // set pos vector
    mtrx[0][3] = xyz.X();
    mtrx[1][3] = xyz.Y();
    mtrx[2][3] = xyz.Z();
    // NOLINTEND

    transformations.push_back(mtrx);
}

void TKJtReader::readShapeVertex(const Handle(JtNode_Shape_Vertex) & aShape)
{
    const JtData_Object::VectorOfLateLoads& aLateLoaded = aShape->LateLoads();
    for (std::size_t index = 0; index < aLateLoaded.Count(); ++index) {
        Handle(JtData_Object) anObject = aLateLoaded[index]->DefferedObject();
        if (anObject.IsNull()) {
            aLateLoaded[index]->Load();
            anObject = aLateLoaded[index]->DefferedObject();
        }
        if (!anObject.IsNull()) {
            Handle(JtElement_ShapeLOD_TriStripSet) aLOD =
                Handle(JtElement_ShapeLOD_TriStripSet)::DownCast(anObject);
            if (!aLOD.IsNull()) {
                getTriangleStripSet(aLOD);
            }
        }
    }
}

void TKJtReader::getTriangleStripSet(const Handle(JtElement_ShapeLOD_TriStripSet) & aLOD)
{
    const int pointsPerFace = 3;
    std::vector<Base::Vector3f> points;
    std::vector<int> faces;
    const JtElement_ShapeLOD_Vertex::VertexData& vertices = aLOD->Vertices();
    const JtElement_ShapeLOD_Vertex::IndicesVec& indices = aLOD->Indices();
    float* data = vertices.Data();
    // NOLINTBEGIN
    for (int index = 0; index < indices.Count(); index += 3) {
        int coordIndex = indices[index] * 3;
        points.emplace_back(data[coordIndex], data[coordIndex + 1], data[coordIndex + 2]);
        coordIndex = indices[index + 1] * 3;
        points.emplace_back(data[coordIndex], data[coordIndex + 1], data[coordIndex + 2]);
        coordIndex = indices[index + 2] * 3;
        points.emplace_back(data[coordIndex], data[coordIndex + 1], data[coordIndex + 2]);
        faces.push_back(pointsPerFace);
    }
    // NOLINTEND
    builder.addNode(Base::Coordinate3Item {points});
    builder.addNode(Base::FaceSetItem {faces});
}

void TKJtReader::readPartition(const Handle(JtNode_Partition) & aPart)
{
    if (rootNode != aPart) {
        TCollection_ExtendedString aFullPath = jtDir + aPart->FileName();
        TCollection_AsciiString aFileName(aFullPath);
        Handle(JtData_Model) aModel = new JtData_Model(aFullPath);
        Handle(JtNode_Partition) aNode = aModel->Init();
        if (!aNode.IsNull()) {
            builder.addNode(Base::InfoItem(std::string(aFileName.ToCString())));
            traverseGraph(aNode);
        }
    }
}

void TKJtReader::readGroup(const Handle(JtNode_Group) & aGroup)
{
    if (!transformations.empty()) {
        builder.addNode(Base::TransformItem {transformations.back()});
        transformations.pop_back();
    }
    const auto& aChildren = aGroup->Children();
    for (std::size_t aChildIdx = 0; aChildIdx < aChildren.Count(); ++aChildIdx) {
        Handle(JtNode_Base) aChild = Handle(JtNode_Base)::DownCast(aChildren[aChildIdx]);
        traverseGraph(aChild);
    }
}

void TKJtReader::readRangeLOD(const Handle(JtNode_RangeLOD) & aRangeLOD)
{
    readGroup(aRangeLOD);
}

void TKJtReader::readInstance(const Handle(JtNode_Instance) & anInstance)
{
    Handle(JtNode_Base) aBase = Handle(JtNode_Base)::DownCast(anInstance->Object());
    if (!aBase.IsNull()) {
        traverseGraph(aBase);
    }
}

void TKJtReader::readAttributes(const JtData_Object::VectorOfObjects& attr)
{
    for (std::size_t aAttrIdx = 0; aAttrIdx < attr.Count(); ++aAttrIdx) {
        Handle(JtData_Object) anAttr = attr[aAttrIdx];
        if (anAttr->IsKind(Type_JtAttribute_GeometricTransform)) {
            Handle(JtAttribute_GeometricTransform) aTransform =
                Handle(JtAttribute_GeometricTransform)::DownCast(anAttr);
            readTransformAttribute(aTransform);
        }
        else if (anAttr->IsKind(Type_JtAttribute_Material)) {
            Handle(JtAttribute_Material) aMaterial = Handle(JtAttribute_Material)::DownCast(anAttr);
            readMaterialAttribute(aMaterial);
        }
    }
}

void TKJtReader::traverseGraph(const Handle(JtNode_Base) & aNode)
{
    readAttributes(aNode->Attributes());

    if (aNode->IsKind(Type_JtNode_Partition)) {
        Handle(JtNode_Partition) aPart = Handle(JtNode_Partition)::DownCast(aNode);
        builder.beginSeparator();
        readPartition(aPart);
        readGroup(aPart);
        builder.endSeparator();
    }
    else if (aNode->IsKind(Type_JtNode_RangeLOD)) {
        Handle(JtNode_RangeLOD) aRangeLOD = Handle(JtNode_RangeLOD)::DownCast(aNode);
        builder.beginSeparator();
        readRangeLOD(aRangeLOD);
        builder.endSeparator();
    }
    else if (aNode->IsKind(Type_JtNode_Group)) {
        Handle(JtNode_Group) aGroup = Handle(JtNode_Group)::DownCast(aNode);
        builder.beginSeparator();
        readGroup(aGroup);
        builder.endSeparator();
    }
    else if (aNode->IsKind(Type_JtNode_Instance)) {
        Handle(JtNode_Instance) anInstance = Handle(JtNode_Instance)::DownCast(aNode);
        readInstance(anInstance);
    }
    else if (aNode->IsKind(Type_JtNode_Shape_Vertex)) {
        Handle(JtNode_Shape_Vertex) aShape = Handle(JtNode_Shape_Vertex)::DownCast(aNode);
        readShapeVertex(aShape);
    }
}
