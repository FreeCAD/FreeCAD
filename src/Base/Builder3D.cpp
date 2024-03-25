/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <algorithm>
#include <cassert>
#include <exception>
#include <string>
#include <string_view>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#endif

#include "Builder3D.h"
#include "Console.h"
#include "Exception.h"
#include "FileInfo.h"
#include "Matrix.h"
#include "Stream.h"
#include "Tools.h"


using namespace Base;


constexpr float valueMinLegal {-1.0F};
constexpr float valueMaxLegal {1.0F};

ColorRGB::ColorRGB()
    : Rgb {1.0F, 1.0F, 1.0F}
{}

ColorRGB::ColorRGB(float red, float green, float blue)
    : Rgb {valueInRange(red), valueInRange(green), valueInRange(blue)}
{}

float ColorRGB::valueInRange(float value)
{
    return std::clamp(value, valueMinLegal, valueMaxLegal);
}

const char* DrawStyle::styleAsString() const
{
    switch (style) {
        case Style::Filled:
            return "FILLED";
        case Style::Lines:
            return "LINES";
        case Style::Points:
            return "POINTS";
        case Style::Invisible:
            return "INVISIBLE";
    }
    return "FILLED";
}

std::string DrawStyle::patternAsString() const
{
    std::stringstream str;
    str << "0x" << std::hex << linePattern;
    return str.str();
}

const char* VertexOrdering::toString() const
{
    switch (ordering) {
        case Ordering::UnknownOrdering:
            return "UNKNOWN_ORDERING";
        case Ordering::Clockwise:
            return "CLOCKWISE";
        case Ordering::CounterClockwise:
            return "COUNTERCLOCKWISE";
    }
    return "UNKNOWN_ORDERING";
}

const char* ShapeType::toString() const
{
    switch (type) {
        case Type::UnknownShapeType:
            return "UNKNOWN_SHAPE_TYPE";
        case Type::Convex:
            return "SOLID";
    }
    return "UNKNOWN_SHAPE_TYPE";
}

const char* BindingElement::bindingAsString() const
{
    switch (value) {
        case Binding::PerPart:
            return "PER_PART";
        case Binding::PerPartIndexed:
            return "PER_PART_INDEXED";
        case Binding::PerFace:
            return "PER_FACE";
        case Binding::PerFaceIndexed:
            return "PER_FACE_INDEXED";
        case Binding::PerVertex:
            return "PER_VERTEX";
        case Binding::PerVertexIndexed:
            return "PER_VERTEX_INDEXED";
        default:
            return "OVERALL";
    }
}

const char* PolygonOffset::styleAsString() const
{
    switch (style) {
        case Style::Filled:
            return "FILLED";
        case Style::Lines:
            return "LINES";
        case Style::Points:
            return "POINTS";
    }
    return "FILLED";
}

// -----------------------------------------------------------------------------

InventorOutput::InventorOutput(std::ostream& result, Indentation& indent)
    : result(result)
    , indent(indent)
{}

std::ostream& InventorOutput::stream()
{
    return result;
}

std::ostream& InventorOutput::write()
{
    result << indent;
    return result;
}

std::ostream& InventorOutput::write(const char* str)
{
    result << indent << str;
    return result;
}

std::ostream& InventorOutput::write(const std::string& str)
{
    result << indent << str;
    return result;
}

std::ostream& InventorOutput::writeLine()
{
    result << indent << '\n';
    return result;
}

std::ostream& InventorOutput::writeLine(const char* str)
{
    result << indent << str << '\n';
    return result;
}

std::ostream& InventorOutput::writeLine(const std::string& str)
{
    result << indent << str << '\n';
    return result;
}

void InventorOutput::increaseIndent()
{
    indent.increaseIndent();
}

void InventorOutput::decreaseIndent()
{
    indent.decreaseIndent();
}

// -----------------------------------------------------------------------------

namespace Base
{
template<class type>
struct field_traits
{
};

template<>
struct field_traits<float>
{
    using field_type = float;
    static std::ostream& write(std::ostream& out, const field_type& item)
    {
        out << item;
        return out;
    }
};

template<>
struct field_traits<Vector3f>
{
    using field_type = Vector3f;
    static std::ostream& write(std::ostream& out, const field_type& item)
    {
        out << item.x << " " << item.y << " " << item.z;
        return out;
    }
};

template<>
struct field_traits<ColorRGB>
{
    using field_type = ColorRGB;
    static std::ostream& write(std::ostream& out, const field_type& item)
    {
        out << item.red() << " " << item.green() << " " << item.blue();
        return out;
    }
};

/**
 * Writes a field type to a stream.
 * @author Werner Mayer
 */
class InventorFieldWriter
{
public:
    template<typename T>
    void write(const char* fieldName, const std::vector<T>& fieldData, InventorOutput& out) const;
};

template<typename T>
void InventorFieldWriter::write(const char* fieldName,
                                const std::vector<T>& fieldData,
                                InventorOutput& out) const
{
    if (fieldData.empty()) {
        return;
    }

    if (fieldData.size() == 1) {
        out.write() << fieldName << " ";
        field_traits<T>::write(out.stream(), fieldData[0]) << '\n';
    }
    else {
        out.write() << fieldName << " [\n";
        out.increaseIndent();
        for (auto it : fieldData) {
            out.write();
            field_traits<T>::write(out.stream(), it) << '\n';
        }
        out.decreaseIndent();
        out.write() << "]\n";
    }
}

template<>
void InventorFieldWriter::write<int>(const char* fieldName,
                                     const std::vector<int>& fieldData,
                                     InventorOutput& out) const
{
    if (fieldData.empty()) {
        return;
    }

    out.write() << fieldName << " [\n";
    out.increaseIndent();
    std::size_t last_index {fieldData.size()};
    std::size_t index {};
    for (auto it : fieldData) {
        if (index % 8 == 0) {
            out.write();
        }
        if (index < last_index) {
            out.stream() << it << ", ";
        }
        else {
            out.stream() << it << " ] \n";
        }
        if (++index % 8 == 0) {
            out.stream() << '\n';
        }
    }
    out.decreaseIndent();
    out.write() << "]\n";
}
}  // namespace Base

// -----------------------------------------------------------------------------

LabelItem::LabelItem(std::string text)
    : text(std::move(text))
{}

void LabelItem::write(InventorOutput& out) const
{
    out.write("Label {\n");
    out.write() << "  label \"" << text << "\"\n";
    out.write("}\n");
}

// -----------------------------------------------------------------------------

InfoItem::InfoItem(std::string text)
    : text(std::move(text))
{}

void InfoItem::write(InventorOutput& out) const
{
    out.write("Info {\n");
    out.write() << "  string \"" << text << "\"\n";
    out.write("}\n");
}

// -----------------------------------------------------------------------------

BaseColorItem::BaseColorItem(const ColorRGB& rgb)
    : rgb(rgb)
{}

void BaseColorItem::write(InventorOutput& out) const
{
    out.write("BaseColor {\n");
    out.write() << "  rgb " << rgb.red() << " " << rgb.green() << " " << rgb.blue() << '\n';
    out.write("}\n");
}

// -----------------------------------------------------------------------------

PointItem::PointItem(const Base::Vector3f& point, DrawStyle drawStyle, const ColorRGB& rgb)
    : point(point)
    , drawStyle(drawStyle)
    , rgb(rgb)
{}

void PointItem::write(InventorOutput& out) const
{
    out.write() << "Separator { \n";
    out.write() << "  Material { \n";
    out.write() << "    diffuseColor " << rgb.red() << " " << rgb.green() << " " << rgb.blue()
                << '\n';
    out.write() << "  }\n";
    out.write() << "  MaterialBinding { value PER_PART }\n";
    out.write() << "  DrawStyle { pointSize " << drawStyle.pointSize << "}\n";
    out.write() << "  Coordinate3 {\n";
    out.write() << "    point [ " << point.x << " " << point.y << " " << point.z << "]\n";
    out.write() << "  }\n";
    out.write() << "  PointSet { }\n";
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

LineItem::LineItem(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& rgb)
    : line(line)
    , drawStyle(drawStyle)
    , rgb(rgb)
{}

void LineItem::write(InventorOutput& out) const
{
    std::string pattern = drawStyle.patternAsString();

    out.write("  Separator { \n");
    out.write() << "    Material { diffuseColor " << rgb.red() << " " << rgb.green() << " "
                << rgb.blue() << "} \n";
    out.write() << "    DrawStyle { lineWidth " << drawStyle.lineWidth << " linePattern " << pattern
                << " } \n";
    out.write() << "    Coordinate3 { \n";
    out.write() << "      point [ ";
    out.write() << line.p1.x << " " << line.p1.y << " " << line.p1.z << ",";
    out.write() << line.p2.x << " " << line.p2.y << " " << line.p2.z;
    out.write() << " ] \n";
    out.write() << "    } \n";
    out.write() << "    LineSet { } \n";
    out.write() << "  } \n";
}

// -----------------------------------------------------------------------------

MultiLineItem::MultiLineItem(std::vector<Vector3f> points, DrawStyle drawStyle, const ColorRGB& rgb)
    : points {std::move(points)}
    , drawStyle {drawStyle}
    , rgb {rgb}
{}

void MultiLineItem::write(InventorOutput& out) const
{
    std::string pattern = drawStyle.patternAsString();

    out.write() << "Separator {\n";
    out.write() << "  Material { diffuseColor " << rgb.red() << " " << rgb.green() << " "
                << rgb.blue() << "}\n";
    out.write() << "  DrawStyle { lineWidth " << drawStyle.lineWidth << " linePattern " << pattern
                << " }\n";
    out.write() << "  Coordinate3 {\n";

    InventorFieldWriter writer;
    writer.write<Vector3f>("point", points, out);

    out.write() << "  }\n";
    out.write() << "  LineSet {\n";
    out.write() << "    numVertices [ -1 ]\n";
    out.write() << "  }\n";
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

ArrowItem::ArrowItem(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& rgb)
    : line(line)
    , drawStyle(drawStyle)
    , rgb(rgb)
{}

void ArrowItem::write(InventorOutput& out) const
{
    float length = line.Length();
    float coneLength = length / 10.0F;
    float coneRadius = coneLength / 2.0F;
    float sf1 = length - coneLength;
    float sf2 = length - coneLength / 2.0F;

    Vector3f dir = line.GetDirection();
    dir.Normalize();
    dir.Scale(sf1, sf1, sf1);
    Vector3f pt2s = line.p1 + dir;
    dir.Normalize();
    dir.Scale(sf2, sf2, sf2);
    Vector3f cpt = line.p1 + dir;

    Vector3f rot = Vector3f(0.0F, 1.0F, 0.0F) % dir;
    rot.Normalize();
    float angle = Vector3f(0.0F, 1.0F, 0.0F).GetAngle(dir);

    out.write() << "Separator {\n";
    out.write() << "  Material { diffuseColor " << rgb.red() << " " << rgb.green() << " "
                << rgb.blue() << "}\n";
    out.write() << "  DrawStyle { lineWidth " << drawStyle.lineWidth << " }\n";
    out.write() << "  Coordinate3 {\n";
    out.write() << "    point [ ";
    out.write() << line.p1.x << " " << line.p1.y << " " << line.p1.z << ", ";
    out.write() << pt2s.x << " " << pt2s.y << " " << pt2s.z;
    out.write() << " ]\n";
    out.write() << "  }\n";
    out.write() << "  LineSet { }\n";
    out.write() << "  Transform { \n";
    out.write() << "    translation " << cpt.x << " " << cpt.y << " " << cpt.z << '\n';
    out.write() << "    rotation " << rot.x << " " << rot.y << " " << rot.z << " " << angle << '\n';
    out.write() << "  }\n";
    out.write() << "  Cone { bottomRadius " << coneRadius << " height " << coneLength << "} \n";
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

BoundingBoxItem::BoundingBoxItem(const Vector3f& pt1,
                                 const Vector3f& pt2,
                                 DrawStyle drawStyle,
                                 const ColorRGB& rgb)
    : pt1 {pt1}
    , pt2 {pt2}
    , drawStyle {drawStyle}
    , rgb {rgb}
{}

void BoundingBoxItem::write(InventorOutput& out) const
{
    std::vector<Base::Vector3f> points(8);
    points[0].Set(pt1.x, pt1.y, pt1.z);
    points[1].Set(pt1.x, pt1.y, pt2.z);
    points[2].Set(pt1.x, pt2.y, pt1.z);
    points[3].Set(pt1.x, pt2.y, pt2.z);
    points[4].Set(pt2.x, pt1.y, pt1.z);
    points[5].Set(pt2.x, pt1.y, pt2.z);
    points[6].Set(pt2.x, pt2.y, pt1.z);
    points[7].Set(pt2.x, pt2.y, pt2.z);

    std::vector<int> lineset = {0, 2, 6,  4, 0, -1, 1, 5, 7,  3, 1, -1, 7, 6, 2,
                                3, 7, -1, 3, 2, 0,  1, 3, -1, 5, 1, 0,  4, 5, -1};

    out.write() << "Separator {\n";
    out.write() << "  Material { diffuseColor " << rgb.red() << " " << rgb.green() << " "
                << rgb.blue() << "}\n";
    out.write() << "  DrawStyle { lineWidth " << drawStyle.lineWidth << "}\n";

    Coordinate3Item coords {points};
    out.increaseIndent();
    coords.write(out);
    out.decreaseIndent();

    IndexedLineSetItem indexed {lineset};
    out.increaseIndent();
    indexed.write(out);
    out.decreaseIndent();

    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

void MaterialItem::setAmbientColor(const std::vector<ColorRGB>& rgb)
{
    ambientColor = rgb;
}

void MaterialItem::setDiffuseColor(const std::vector<ColorRGB>& rgb)
{
    diffuseColor = rgb;
}

void MaterialItem::setSpecularColor(const std::vector<ColorRGB>& rgb)
{
    specularColor = rgb;
}

void MaterialItem::setEmissiveColor(const std::vector<ColorRGB>& rgb)
{
    emissiveColor = rgb;
}

void MaterialItem::setShininess(const std::vector<float>& value)
{
    shininess = value;
}

void MaterialItem::setTransparency(const std::vector<float>& value)
{
    transparency = value;
}

void MaterialItem::write(InventorOutput& out) const
{
    beginMaterial(out);
    writeAmbientColor(out);
    writeDiffuseColor(out);
    writeSpecularColor(out);
    writeEmissiveColor(out);
    writeShininess(out);
    writeTransparency(out);
    endMaterial(out);
}

void MaterialItem::beginMaterial(InventorOutput& out) const
{
    out.writeLine("Material {");
    out.increaseIndent();
}

void MaterialItem::endMaterial(InventorOutput& out) const
{
    out.decreaseIndent();
    out.writeLine("}");
}

void MaterialItem::writeAmbientColor(InventorOutput& out) const
{
    InventorFieldWriter writer;
    writer.write<ColorRGB>("ambientColor", ambientColor, out);
}

void MaterialItem::writeDiffuseColor(InventorOutput& out) const
{
    InventorFieldWriter writer;
    writer.write<ColorRGB>("diffuseColor", diffuseColor, out);
}

void MaterialItem::writeSpecularColor(InventorOutput& out) const
{
    InventorFieldWriter writer;
    writer.write<ColorRGB>("specularColor", specularColor, out);
}

void MaterialItem::writeEmissiveColor(InventorOutput& out) const
{
    InventorFieldWriter writer;
    writer.write<ColorRGB>("emissiveColor", emissiveColor, out);
}

void MaterialItem::writeShininess(InventorOutput& out) const
{
    InventorFieldWriter writer;
    writer.write<float>("shininess", shininess, out);
}

void MaterialItem::writeTransparency(InventorOutput& out) const
{
    InventorFieldWriter writer;
    writer.write<float>("transparency", transparency, out);
}

// -----------------------------------------------------------------------------

MaterialBindingItem::MaterialBindingItem(BindingElement::Binding bind)
{
    value.value = bind;
}

void MaterialBindingItem::setValue(BindingElement::Binding bind)
{
    value.value = bind;
}

void MaterialBindingItem::write(InventorOutput& out) const
{
    out.write() << "MaterialBinding { value " << value.bindingAsString() << " } \n";
}

// -----------------------------------------------------------------------------

DrawStyleItem::DrawStyleItem(DrawStyle value)
    : style {value}
{}

void DrawStyleItem::setValue(DrawStyle value)
{
    style = value;
}

void DrawStyleItem::write(InventorOutput& out) const
{
    out.write() << "DrawStyle {\n";
    out.write() << "  style " << style.styleAsString() << '\n';
    out.write() << "  pointSize " << style.pointSize << '\n';
    out.write() << "  lineWidth " << style.lineWidth << '\n';
    out.write() << "  linePattern " << style.patternAsString() << '\n';
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

ShapeHintsItem::ShapeHintsItem(float creaseAngle)
    : creaseAngle(creaseAngle)
{}

void ShapeHintsItem::setVertexOrdering(VertexOrdering::Ordering value)
{
    vertexOrdering.ordering = value;
}

void ShapeHintsItem::setShapeType(ShapeType::Type value)
{
    shapeType.type = value;
}

void ShapeHintsItem::write(InventorOutput& out) const
{
    out.write() << "ShapeHints {\n";
    out.write() << "  creaseAngle " << creaseAngle << '\n';
    out.write() << "  vertexOrdering " << vertexOrdering.toString() << '\n';
    out.write() << "  shapeType " << shapeType.toString() << '\n';
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

void PolygonOffsetItem::setValue(PolygonOffset value)
{
    offset = value;
}

void PolygonOffsetItem::write(InventorOutput& out) const
{
    out.write() << "PolygonOffset {\n";
    out.write() << "  factor " << offset.factor << '\n';
    out.write() << "  units " << offset.units << '\n';
    out.write() << "  styles " << offset.styleAsString() << '\n';
    out.write() << "  on " << (offset.on ? "TRUE" : "FALSE") << '\n';
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

Coordinate3Item::Coordinate3Item(std::vector<Vector3f> points)
    : points(std::move(points))
{}

void Coordinate3Item::write(InventorOutput& out) const
{
    beginPoint(out);
    InventorFieldWriter writer;
    writer.write<Vector3f>("point", points, out);
    endPoint(out);
}

void Coordinate3Item::beginPoint(InventorOutput& out) const
{
    out.writeLine("Coordinate3 {");
    out.increaseIndent();
}

void Coordinate3Item::endPoint(InventorOutput& out) const
{
    out.decreaseIndent();
    out.writeLine("}");
}

// -----------------------------------------------------------------------------

void PointSetItem::write(InventorOutput& out) const
{
    out.writeLine("PointSet { }");
}

// -----------------------------------------------------------------------------

void LineSetItem::write(InventorOutput& out) const
{
    out.writeLine("LineSet { }");
}

// -----------------------------------------------------------------------------

FaceSetItem::FaceSetItem(std::vector<int> indices)
    : indices(std::move(indices))
{}

void FaceSetItem::write(InventorOutput& out) const
{
    out.write() << "FaceSet {\n";
    out.increaseIndent();
    InventorFieldWriter writer;
    writer.write<int>("numVertices", indices, out);
    out.decreaseIndent();
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

IndexedLineSetItem::IndexedLineSetItem(std::vector<int> indices)
    : indices(std::move(indices))
{}

void IndexedLineSetItem::write(InventorOutput& out) const
{
    out.write() << "IndexedLineSet {\n";
    out.increaseIndent();
    InventorFieldWriter writer;
    writer.write<int>("coordIndex", indices, out);
    out.decreaseIndent();
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

IndexedFaceSetItem::IndexedFaceSetItem(std::vector<int> indices)
    : indices(std::move(indices))
{}

void IndexedFaceSetItem::write(InventorOutput& out) const
{
    out.write() << "IndexedFaceSet {\n";
    out.increaseIndent();
    InventorFieldWriter writer;
    writer.write<int>("coordIndex", indices, out);
    out.decreaseIndent();
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

NormalItem::NormalItem(std::vector<Base::Vector3f> vec)
    : vector(std::move(vec))
{}

void NormalItem::write(InventorOutput& out) const
{
    beginNormal(out);
    InventorFieldWriter writer;
    writer.write<Vector3f>("vector", vector, out);
    endNormal(out);
}

void NormalItem::beginNormal(InventorOutput& out) const
{
    out.writeLine("Normal {");
    out.increaseIndent();
}

void NormalItem::endNormal(InventorOutput& out) const
{
    out.decreaseIndent();
    out.writeLine("}");
}

// -----------------------------------------------------------------------------

void NormalBindingItem::setValue(BindingElement::Binding bind)
{
    value.value = bind;
}

void NormalBindingItem::write(InventorOutput& out) const
{
    out.write() << "NormalBinding { value " << value.bindingAsString() << " }\n";
}

// -----------------------------------------------------------------------------

void CylinderItem::setRadius(float value)
{
    radius = value;
}

void CylinderItem::setHeight(float value)
{
    height = value;
}

void CylinderItem::write(InventorOutput& out) const
{
    out.write() << "Cylinder {\n";
    out.write() << "  radius " << radius << "\n";
    out.write() << "  height " << height << "\n";
    out.write() << "  parts (SIDES | TOP | BOTTOM)\n";
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

void ConeItem::setBottomRadius(float value)
{
    bottomRadius = value;
}

void ConeItem::setHeight(float value)
{
    height = value;
}

void ConeItem::write(InventorOutput& out) const
{
    out.write() << "Cone { bottomRadius " << bottomRadius << " height " << height << " }\n";
}

// -----------------------------------------------------------------------------

void SphereItem::setRadius(float value)
{
    radius = value;
}

void SphereItem::write(InventorOutput& out) const
{
    out.write() << "Sphere { radius " << radius << " }\n";
}

// -----------------------------------------------------------------------------

void NurbsSurfaceItem::setControlPoints(int numU, int numV)
{
    numUControlPoints = numU;
    numVControlPoints = numV;
}

void NurbsSurfaceItem::setKnotVector(const std::vector<float>& uKnots,
                                     const std::vector<float>& vKnots)
{
    uKnotVector = uKnots;
    vKnotVector = vKnots;
}

void NurbsSurfaceItem::write(InventorOutput& out) const
{
    out.write() << "NurbsSurface {\n";
    out.write() << "  numUControlPoints " << numUControlPoints << '\n';
    out.write() << "  numVControlPoints " << numVControlPoints << '\n';
    out.increaseIndent();
    InventorFieldWriter writer;
    writer.write<float>("uKnotVector", uKnotVector, out);
    writer.write<float>("vKnotVector", vKnotVector, out);
    out.decreaseIndent();
    out.write() << "}\n";
}

// -----------------------------------------------------------------------------

Text2Item::Text2Item(std::string string)
    : string(std::move(string))
{}

void Text2Item::write(InventorOutput& out) const
{
    out.write() << "Text2 { string \"" << string << "\" " << "}\n";
}

// -----------------------------------------------------------------------------

// NOLINTNEXTLINE
TransformItem::TransformItem(const Base::Placement& placement)
    : placement(placement)
{}

TransformItem::TransformItem(const Matrix4D& transform)
{
    placement.fromMatrix(transform);
}

void TransformItem::write(InventorOutput& out) const
{
    Base::Vector3d translation = placement.getPosition();
    Base::Vector3d rotationaxis;
    double angle {};
    placement.getRotation().getValue(rotationaxis, angle);

    out.write() << "Transform {\n";
    out.write() << "  translation " << translation.x << " " << translation.y << " " << translation.z
                << '\n';
    out.write() << "  rotation " << rotationaxis.x << " " << rotationaxis.y << " " << rotationaxis.z
                << " " << angle << '\n';
    out.write() << "}" << '\n';
}

// -----------------------------------------------------------------------------

InventorBuilder::InventorBuilder(std::ostream& str)
    : result(str)
{
    addHeader();
}

InventorBuilder::~InventorBuilder() = default;

void InventorBuilder::addHeader()
{
    result << "#Inventor V2.1 ascii \n\n";
}

void InventorBuilder::increaseIndent()
{
    indent.increaseIndent();
}

void InventorBuilder::decreaseIndent()
{
    indent.decreaseIndent();
}

void InventorBuilder::addNode(const NodeItem& node)
{
    InventorOutput out(result, indent);
    node.write(out);
}

void InventorBuilder::beginSeparator()
{
    result << indent << "Separator { \n";
    increaseIndent();
}

void InventorBuilder::endSeparator()
{
    decreaseIndent();
    result << indent << "}\n";
}

// -----------------------------------------------------------------------------

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Builder3D::Builder3D()
    : result {}
    , builder {result}
{}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Builder3D::~Builder3D() = default;

void Builder3D::clear()
{
    // under gcc stringstream::str() returns a copy not a reference
#if defined(_MSC_VER)
    result.str().clear();
#endif
    result.clear();
}

/**
 * Save the resulting inventor 3D representation to the Console().Log() facility.
 * In DEBUG mode the Gui (if running) will trigger on that and show the representation in
 * the active Viewer/Document. It shows only one representation on time. If you need to
 * show more then one representation use saveToFile() instead.
 * @see saveToFile()
 */
void Builder3D::saveToLog()
{
    ILogger* obs = Base::Console().Get("StatusBar");
    if (obs) {
        obs->SendLog("Builder3D",
                     result.str(),
                     Base::LogStyle::Log,
                     Base::IntendedRecipient::Developer,
                     Base::ContentType::Untranslatable);
    }
}

/**
 * Save the resulting inventor 3D representation to a file. Ending should be *.iv.
 * That enables you to show the result in a Inventor Viewer or in FreeCAD by:
 * /code
 * Gui.document().addAnnotation("Debug","MyFile.iv")
 * /endcode
 * @see saveToFile()
 */
void Builder3D::saveToFile(const char* FileName)
{
    Base::FileInfo fi(FileName);
    Base::ofstream file(fi);
    if (!file) {
        throw FileException("Cannot open file");
    }

    file << result.str();
}

void Builder3D::addNode(const NodeItem& item)
{
    builder.addNode(item);
}

void Builder3D::beginSeparator()
{
    builder.beginSeparator();
}

void Builder3D::endSeparator()
{
    builder.endSeparator();
}

// -----------------------------------------------------------------------------

template<typename T>
std::vector<T> InventorLoader::readData(const char* fieldName) const
{
    std::vector<T> fieldValues;
    std::string str;

    // search for 'fieldName' and '['
    bool found = false;
    while (std::getline(inp, str)) {
        std::string::size_type point = str.find(fieldName);
        std::string::size_type open = str.find('[');
        if (point != std::string::npos && open > point) {
            str = str.substr(open);
            found = true;
            break;
        }
    }

    if (!found) {
        return {};
    }

    do {
        boost::char_separator<char> sep(" ,");
        boost::tokenizer<boost::char_separator<char>> tokens(str, sep);
        std::vector<std::string> token_results;
        token_results.assign(tokens.begin(), tokens.end());

        for (const auto& it : token_results) {
            try {
                T value = boost::lexical_cast<T>(it);
                fieldValues.emplace_back(value);
            }
            catch (const boost::bad_lexical_cast&) {
            }
        }

        // search for ']' to finish the reading
        if (str.find(']') != std::string::npos) {
            break;
        }
    } while (std::getline(inp, str));

    return fieldValues;
}

std::vector<Vector3f> InventorLoader::convert(const std::vector<float>& data) const
{
    if (data.size() % 3 != 0) {
        throw std::string("Reading failed");
    }

    std::size_t len = data.size() / 3;
    std::vector<Vector3f> points;
    points.reserve(len);

    for (std::size_t i = 0; i < len; i++) {
        float x = data[3 * i];
        float y = data[3 * i + 1];
        float z = data[3 * i + 2];
        points.emplace_back(x, y, z);
    }

    return points;
}

std::vector<InventorLoader::Face> InventorLoader::convert(const std::vector<int32_t>& data) const
{
    std::vector<Face> faces;
    faces.reserve(data.size());
    int32_t coordIndex = 0;
    for (const auto it : data) {
        if (it == 3) {
            faces.emplace_back(coordIndex, coordIndex + 1, coordIndex + 2);
        }
        else if (it == 4) {
            faces.emplace_back(coordIndex, coordIndex + 1, coordIndex + 2);
            faces.emplace_back(coordIndex, coordIndex + 2, coordIndex + 3);
        }
        coordIndex += it;
    }
    return faces;
}

std::vector<std::vector<int32_t>> InventorLoader::split(const std::vector<int32_t>& data)
{
    std::vector<std::vector<int32_t>> splitdata;
    std::vector<int32_t>::const_iterator begin = data.cbegin();
    std::vector<int32_t>::const_iterator it = begin;

    while ((it = std::find(begin, data.cend(), -1)) != data.cend()) {
        splitdata.emplace_back(begin, it);
        begin = it;
        std::advance(begin, 1);
    }
    return splitdata;
}

std::vector<InventorLoader::Face>
InventorLoader::convert(const std::vector<std::vector<int32_t>>& coordIndex) const
{
    std::vector<Face> faces;
    faces.reserve(coordIndex.size());
    for (const auto& it : coordIndex) {
        if (it.size() == 3) {
            faces.emplace_back(it[0], it[1], it[2]);
        }
        else if (it.size() == 4) {
            faces.emplace_back(it[0], it[1], it[2]);
            faces.emplace_back(it[0], it[2], it[3]);
        }
    }
    return faces;
}

void InventorLoader::readNormals()
{
    auto data = readData<float>("vector");
    vector = convert(data);
}

void InventorLoader::readCoords()
{
    auto data = readData<float>("point");
    points = convert(data);
}

void InventorLoader::readIndexedFaceSet()
{
    auto data = readData<int32_t>("coordIndex");
    faces = convert(split(data));
}

void InventorLoader::readFaceSet()
{
    auto data = readData<int32_t>("numVertices");
    faces = convert(data);
    isnonindexed = true;
}

bool InventorLoader::read()
{
    if (!inp || inp.bad()) {
        return false;
    }

    std::string line;

    // Verify it's an Inventor 2.1 file
    std::getline(inp, line);
    if (line.find("#Inventor V2.1 ascii") == std::string::npos) {
        return false;
    }

    while (std::getline(inp, line)) {
        // read the normals if they are defined
        if (line.find("Normal {") != std::string::npos) {
            readNormals();
        }
        else if (line.find("Coordinate3 {") != std::string::npos) {
            readCoords();
        }
        else if (line.find("IndexedFaceSet {") != std::string::npos) {
            readIndexedFaceSet();
            break;
        }
        else if (line.find("FaceSet {") != std::string::npos) {
            readFaceSet();
            break;
        }
    }
    return true;
}

bool InventorLoader::isValid() const
{
    int32_t value {static_cast<int32_t>(points.size())};
    auto inRange = [value](const Face& face) {
        if (face.p1 < 0 || face.p1 >= value) {
            return false;
        }
        if (face.p2 < 0 || face.p2 >= value) {
            return false;
        }
        if (face.p3 < 0 || face.p3 >= value) {
            return false;
        }
        return true;
    };

    return std::all_of(faces.cbegin(), faces.cend(), [&inRange](const Face& face) {
        return inRange(face);
    });
}

namespace Base
{
BaseExport Vector3f to_vector(std::string str)
{
    std::string_view view = str;
    if (!boost::starts_with(view, "(") || !boost::ends_with(str, ")")) {
        throw std::runtime_error("string is not a tuple");
    }

    view.remove_prefix(1);
    view.remove_suffix(1);

    str = view;

    boost::char_separator<char> sep(" ,");
    boost::tokenizer<boost::char_separator<char>> tokens(str, sep);
    std::vector<std::string> token_results;
    token_results.assign(tokens.begin(), tokens.end());

    if (token_results.size() != 3) {
        throw std::runtime_error("not a tuple of three floats");
    }

    Base::Vector3f vec;
    vec.x = boost::lexical_cast<float>(token_results.at(0));
    vec.y = boost::lexical_cast<float>(token_results.at(1));
    vec.z = boost::lexical_cast<float>(token_results.at(2));

    return vec;
}

}  // namespace Base
