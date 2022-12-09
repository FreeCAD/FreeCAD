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
# include <algorithm>
# include <cassert>
# include <exception>
# include <string>
# include <string_view>
# include <fstream>
# include <boost/algorithm/string.hpp>
# include <boost/algorithm/string/predicate.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/tokenizer.hpp>
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

ColorRGB::ColorRGB() : Rgb{1.0F, 1.0F, 1.0F}
{
}

ColorRGB::ColorRGB(float red, float green, float blue)
    : Rgb{valueInRange(red),
          valueInRange(green),
          valueInRange(blue)}
{
}

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
{
}

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

namespace Base {
template <class type>
struct field_traits { };

template <>
struct field_traits<float> {
    using field_type = float;
    static std::ostream& write(std::ostream& out, const field_type& item) {
        out << item;
        return out;
    }
};

template <>
struct field_traits<Vector3f> {
    using field_type = Vector3f;
    static std::ostream& write(std::ostream& out, const field_type& item) {
        out << item.x << " " << item.y << " " << item.z;
        return out;
    }
};

template <>
struct field_traits<ColorRGB> {
    using field_type = ColorRGB;
    static std::ostream& write(std::ostream& out, const field_type& item) {
        out << item.red() << " " << item.green() << " " << item.blue();
        return out;
    }
};

/**
 * Writes a field type to a stream.
 * @author Werner Mayer
 */
class InventorFieldWriter {
public:
    template<typename T>
    void write(const char* fieldName, const std::vector<T>& fieldData, InventorOutput& out) const;
};

template<typename T>
void InventorFieldWriter::write(const char* fieldName, const std::vector<T>& fieldData, InventorOutput& out) const
{
    if (fieldData.empty())
        return;

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
}

// -----------------------------------------------------------------------------

LabelItem::LabelItem(const std::string& text) : text(text)
{

}

void LabelItem::write(InventorOutput& out) const
{
    out.write("Label {\n");
    out.write() << "  label \"" << text << "\"\n";
    out.write("}\n");
}

// -----------------------------------------------------------------------------

InfoItem::InfoItem(const std::string& text) : text(text)
{

}

void InfoItem::write(InventorOutput& out) const
{
    out.write("Info {\n");
    out.write() << "  string \"" << text << "\"\n";
    out.write("}\n");
}

// -----------------------------------------------------------------------------

BaseColorItem::BaseColorItem(const ColorRGB& rgb) : rgb(rgb)
{

}

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
{

}

void PointItem::write(InventorOutput& out) const
{
    out.write() << "Separator { \n";
    out.write() << "  Material { \n";
    out.write() << "    diffuseColor " << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << '\n';
    out.write() << "  }\n";
    out.write() << "  MaterialBinding { value PER_PART }\n";
    out.write() << "  DrawStyle { pointSize " << drawStyle.pointSize << "}\n";
    out.write() << "  Coordinate3 {\n";
    out.write() << "    point [ " << point.x << " " << point.y << " " << point.z << "]\n";
    out.write() << "  }\n";
    out.write() << "  PointSet { }\n";
    out.write() <<"}\n";
}

// -----------------------------------------------------------------------------

LineItem::LineItem(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& rgb)
    : line(line)
    , drawStyle(drawStyle)
    , rgb(rgb)
{

}

void LineItem::write(InventorOutput& out) const
{
    std::string pattern = drawStyle.patternAsString();

    out.write("  Separator { \n");
    out.write() << "    Material { diffuseColor " << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n";
    out.write() << "    DrawStyle { lineWidth " << drawStyle.lineWidth << " linePattern " << pattern << " } \n";
    out.write() << "    Coordinate3 { \n";
    out.write() << "      point [ ";
    out.write() <<        line.p1.x << " " << line.p1.y << " " << line.p1.z << ",";
    out.write() <<        line.p2.x << " " << line.p2.y << " " << line.p2.z;
    out.write() << " ] \n";
    out.write() << "    } \n";
    out.write() << "    LineSet { } \n";
    out.write() << "  } \n";
}

// -----------------------------------------------------------------------------

ArrowItem::ArrowItem(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& rgb)
    : line(line)
    , drawStyle(drawStyle)
    , rgb(rgb)
{

}

void ArrowItem::write(InventorOutput& out) const
{
    float length = line.Length();
    float coneLength = length / 10.0F;
    float coneRadius = coneLength / 2.0F;
    float sf1 = length - coneLength;
    float sf2 = length - coneLength/2.0F;

    Vector3f dir = line.GetDirection();
    dir.Normalize();
    dir.Scale(sf1, sf1, sf1);
    Vector3f pt2s = line.p1 + dir;
    dir.Normalize();
    dir.Scale(sf2, sf2, sf2);
    Vector3f cpt = line.p1 + dir;

    Vector3f rot = Vector3f(0.0f, 1.0f, 0.0f) % dir;
    rot.Normalize();
    float angle = Vector3f(0.0f, 1.0f, 0.0f).GetAngle(dir);

    out.write() << "Separator {\n";
    out.write() << "  Material { diffuseColor " << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "}\n";
    out.write() << "  DrawStyle { lineWidth " << drawStyle.lineWidth << " }\n";
    out.write() << "  Coordinate3 {\n";
    out.write() << "    point [ ";
    out.write() <<        line.p1.x << " " << line.p1.y << " " << line.p1.z << ", ";
    out.write() <<        pt2s.x << " " << pt2s.y << " " << pt2s.z;
    out.write() << " ]\n";
    out.write() << "  }\n";
    out.write() << "  LineSet { }\n";
    out.write() << "  Transform { \n";
    out.write() << "    translation "
                << cpt.x << " " << cpt.y << " " << cpt.z << '\n';
    out.write() << "    rotation "
                << rot.x << " " << rot.y << " " << rot.z << " " << angle << '\n';
    out.write() << "  }\n";
    out.write() << "  Cone { bottomRadius " << coneRadius << " height " << coneLength << "} \n";
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

void MaterialBindingItem::setValue(BindingElement::Binding bind)
{
    value.value = bind;
}

void MaterialBindingItem::write(InventorOutput& out) const
{
    out.write() << "MaterialBinding { value "
                << value.bindingAsString() << " } \n";
}

// -----------------------------------------------------------------------------

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

ShapeHintsItem::ShapeHintsItem(float creaseAngle) : creaseAngle(creaseAngle)
{

}

void ShapeHintsItem::write(InventorOutput& out) const
{
    out.write() << "ShapeHints {\n";
    out.write() << "  creaseAngle " << creaseAngle << '\n';
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

Coordinate3Item::Coordinate3Item(const std::vector<Vector3f>& points)
    : points(points)
{
}

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

void NormalItem::setVector(const std::vector<Base::Vector3f>& vec)
{
    vector = vec;
}

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
    out.write() << "NormalBinding { value "
                << value.bindingAsString() << " }\n";
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

InventorBuilder::InventorBuilder(std::ostream& output)
  : result(output)
{
    result << "#Inventor V2.1 ascii \n\n";
}

InventorBuilder:: ~InventorBuilder()
{
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

void InventorBuilder::addInfo(const char* text)
{
    result << indent << "Info { \n";
    result << indent << "  string \"" << text << "\"\n";
    result << indent << "} \n";
}

void InventorBuilder::addLabel(const char* text)
{
    result << indent << "Label { \n";
    result << indent << "  label \"" << text << "\"\n";
    result << indent << "} \n";
}

void InventorBuilder::addBaseColor(const ColorRGB& rgb)
{
    result << indent << "BaseColor { \n";
    result << indent << "  rgb "
           << rgb.red() << " " << rgb.green() << " " << rgb.blue() << '\n';
    result << indent << "} \n";
}

void InventorBuilder::addMaterial(const ColorRGB& rgb, float transparency)
{
    result << indent << "Material { \n";
    result << indent << "  diffuseColor "
           << rgb.red() << " " << rgb.green() << " " << rgb.blue() << '\n';
    if (transparency > 0)
        result << indent << "  transparency " << transparency << '\n';
    result << indent << "} \n";
}

void InventorBuilder::beginMaterial()
{
    result << indent << "Material { \n";
    increaseIndent();
    result << indent << "diffuseColor [\n";
    increaseIndent();
}

void InventorBuilder::endMaterial()
{
    decreaseIndent();
    result << indent << "]\n";
    decreaseIndent();
    result << indent << "}\n";
}

void InventorBuilder::addColor(const ColorRGB& rgb)
{
    result << rgb.red() << " " << rgb.green() << " " << rgb.blue() << '\n';
}

void InventorBuilder::addMaterialBinding(BindingElement bind)
{
    result << indent << "MaterialBinding { value "
           << bind.bindingAsString() << " } \n";
}

void InventorBuilder::addDrawStyle(DrawStyle drawStyle)
{
    result << indent << "DrawStyle {\n"
           << indent << "  style " << drawStyle.styleAsString() << '\n'
           << indent << "  pointSize " << drawStyle.pointSize << '\n'
           << indent << "  lineWidth " << drawStyle.lineWidth << '\n'
           << indent << "  linePattern " << drawStyle.linePattern << '\n'
           << indent << "}\n";
}

void InventorBuilder::addShapeHints(float creaseAngle)
{
    result << indent << "ShapeHints {\n"
           << indent << "  creaseAngle " << creaseAngle << '\n'
           << indent << "}\n";
}

void InventorBuilder::addPolygonOffset(PolygonOffset polygonOffset)
{
    result << indent << "PolygonOffset {\n"
           << indent << "  factor " << polygonOffset.factor << '\n'
           << indent << "  units " << polygonOffset.units << '\n'
           << indent << "  styles " << polygonOffset.styleAsString() << '\n'
           << indent << "  on " << (polygonOffset.on ? "TRUE" : "FALSE") << '\n'
           << indent << "}\n";
}

/**
 * Starts the definition of point set.
 * If possible don't make too many beginPoints() and endPoints() calls.
 * Try to put all points in one set.
 * @see endPoints()
 */
void InventorBuilder::beginPoints()
{
    result << indent << "Coordinate3 { \n";
    increaseIndent();
    result << indent << "point [ \n";
    increaseIndent();
}

/// insert a point in a point set
void InventorBuilder::addPoint(const Vector3f& pnt)
{
    result << indent << pnt.x << " " << pnt.y << " " << pnt.z << ",\n";
}

void InventorBuilder::addPoints(const std::vector<Vector3f>& points)
{
    for (const auto& pnt : points) {
        addPoint(pnt);
    }
}

/**
 * Ends the point set operations and write the resulting inventor string.
 * @see beginPoints()
 */
void InventorBuilder::endPoints()
{
    decreaseIndent();
    result << indent << "]\n";
    decreaseIndent();
    result << indent << "}\n";
}

/**
 * Adds an SoPointSet node after creating an SoCordinate3 node with
 * beginPoints() and endPoints().
 * @see beginPoints()
 * @see endPoints()
 */
void InventorBuilder::addPointSet()
{
    result << indent << "PointSet { } \n";
}

void InventorBuilder::addSinglePoint(const Base::Vector3f &point, DrawStyle drawStyle, const ColorRGB& color)
{
    result << indent << "Separator { ";
    result << indent << "  Material { ";
    result << indent << "    diffuseColor " << color.red() << " "<< color.green() << " "<< color.blue();
    result << indent << "  }";
    result << indent << "  MaterialBinding { value PER_PART } ";
    result << indent << "  DrawStyle { pointSize " << drawStyle.pointSize << "} ";
    result << indent << "  Coordinate3 { ";
    result << indent << "    point [ ";
    result << point.x << " " << point.y << " " << point.z << ",";
    result << indent << "    ] ";
    result << indent << "  }";
    result << indent << "  PointSet { } ";
    result << indent <<"}";
}

/**
 * Adds a SoLineSet node after creating a SoCordinate3 node with
 * beginPoints() and endPoints().
 * @see beginPoints()
 * @see endPoints()
 */
void InventorBuilder::addLineSet()
{
    result << indent << "LineSet { } \n";
}

void InventorBuilder::addText(const char * text)
{
    result << indent << "  Text2 { string \" " << text << "\" " << "} \n";
}

/**
 * Add a Text with a given position to the 3D set. The origin is the
 * lower leftmost corner.
 * @param pos_x,pos_y,pos_z origin of the text
 * @param text the text to display.
 * @param color text color.
 */
void InventorBuilder::addText(const Vector3f& pnt, const char * text, const ColorRGB& rgb)
{
    result << indent << "Separator { \n"
           << indent << "  Material { diffuseColor "
           << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
           << indent << "  Transform { translation "
           << pnt.x << " "<< pnt.y << " "<< pnt.z << "} \n"
           << indent << "  Text2 { string \" " << text << "\" " << "} \n"
           << indent << "}\n";
}

void InventorBuilder::addSingleLine(const Base::Line3f& line, Base::DrawStyle drawStyle, const ColorRGB& rgb)
{
    std::string pattern = drawStyle.patternAsString();

    result << "  Separator { \n"
           << "    Material { diffuseColor " << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
           << "    DrawStyle { lineWidth " << drawStyle.lineWidth << " linePattern " << pattern << " } \n"
           << "    Coordinate3 { \n"
           << "      point [ "
           <<        line.p1.x << " " << line.p1.y << " " << line.p1.z << ","
           <<        line.p2.x << " " << line.p2.y << " " << line.p2.z
           << " ] \n"
           << "    } \n"
           << "    LineSet { } \n"
           << "  } \n";
}

void InventorBuilder::addSingleArrow(const Base::Line3f& line, Base::DrawStyle drawStyle, const ColorRGB& rgb)
{
    float length = line.Length();
    float coneLength = length / 10.0F;
    float coneRadius = coneLength / 2.0F;
    float sf1 = length - coneLength;
    float sf2 = length - coneLength/2.0F;

    Vector3f dir = line.GetDirection();
    dir.Normalize();
    dir.Scale(sf1, sf1, sf1);
    Vector3f pt2s = line.p1 + dir;
    dir.Normalize();
    dir.Scale(sf2, sf2, sf2);
    Vector3f cpt = line.p1 + dir;

    Vector3f rot = Vector3f(0.0f, 1.0f, 0.0f) % dir;
    rot.Normalize();
    float a = Vector3f(0.0f, 1.0f, 0.0f).GetAngle(dir);

    result << indent << "Separator { \n"
           << indent << "  Material { diffuseColor "
           << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
           << indent << "  DrawStyle { lineWidth "
           << drawStyle.lineWidth << "} \n"
           << indent << "  Coordinate3 { \n"
           << indent << "    point [ "
           <<        line.p1.x << " " << line.p1.y << " " << line.p1.z << ","
           <<        pt2s.x << " " << pt2s.y << " " << pt2s.z
           << " ] \n"
           << indent << "  } \n"
           << indent << "  LineSet { } \n"
           << indent << "  Transform { \n"
           << indent << "    translation "
           << cpt.x << " " << cpt.y << " " << cpt.z << " \n"
           << indent << "    rotation "
           << rot.x << " " << rot.y << " " << rot.z << " " << a << '\n'
           << indent << "  } \n"
           << indent << "  Cone { bottomRadius " << coneRadius << " height " << coneLength << "} \n"
           << indent << "} \n";
}

/** Add a line defined by a list of points whereat always a pair (i.e. a point and the following point) builds a line.
 * The size of the list must then be even.
 */
void InventorBuilder::addLineSet(const std::vector<Vector3f>& points, DrawStyle drawStyle, const ColorRGB& rgb)
{
    std::string pattern = drawStyle.patternAsString();

    result << "  Separator { \n"
           << "    Material { diffuseColor " << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
           << "    DrawStyle { lineWidth " << drawStyle.lineWidth << " linePattern " << pattern << " } \n"
           << "    Coordinate3 { \n"
           << "      point [ ";
    std::vector<Vector3f>::const_iterator it = points.begin();
    if ( it != points.end() )
    {
        result << it->x << " " << it->y << " " << it->z;
        for ( ++it ; it != points.end(); ++it )
            result << ",\n          " << it->x << " " << it->y << " " << it->z;
    }

    result << " ] \n"
           << "    } \n"
           << "    LineSet { \n"
           << "      numVertices [ ";
    result << " -1 ";
    result << " ] \n"
           << "    } \n"
           << "  } \n";
}

void InventorBuilder::addIndexedFaceSet(const std::vector<int>& indices)
{
    if (indices.size() < 4)
        return;

    result << indent << "IndexedFaceSet { \n"
           << indent << "  coordIndex [ \n";

    increaseIndent();
    increaseIndent();
    std::vector<int>::const_iterator it_last_f = indices.end()-1;
    int index=0;
    for (std::vector<int>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
        if (index % 8 == 0)
            result << indent;
        if (it != it_last_f)
            result << *it << ", ";
        else
            result << *it << " ] \n";
        if (++index % 8 == 0)
            result << '\n';
    }
    decreaseIndent();
    decreaseIndent();

    result << indent << "} \n";
}

void InventorBuilder::addFaceSet(const std::vector<int>& vertices)
{
    result << indent << "FaceSet { \n"
           << indent << "  numVertices [ \n";

    increaseIndent();
    increaseIndent();
    std::vector<int>::const_iterator it_last_f = vertices.end()-1;
    int index=0;
    for (std::vector<int>::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
        if (index % 8 == 0)
            result << indent;
        if (it != it_last_f)
            result << *it << ", ";
        else
            result << *it << " ] \n";
        if (++index % 8 == 0)
            result << '\n';
    }
    decreaseIndent();
    decreaseIndent();

    result << indent << "} \n";
}

void InventorBuilder::beginNormal()
{
    result << indent << "Normal { \n";
    increaseIndent();
    result << indent << "vector [ \n";
    increaseIndent();
}

void InventorBuilder::endNormal()
{
    decreaseIndent();
    result << indent << "]\n";
    decreaseIndent();
    result << indent << "}\n";
}

void InventorBuilder::addNormalBinding(const char* binding)
{
    result << indent << "NormalBinding {\n"
           << indent << "  value " << binding << '\n'
           << indent << "}\n";
}

void InventorBuilder::addSingleTriangle(const Triangle& triangle, DrawStyle drawStyle, const ColorRGB& rgb)
{
    std::string fs = "";
    if (drawStyle.style == DrawStyle::Style::Filled) {
        fs = "    FaceSet { } ";
    }

    result << "  Separator { \n"
           << "    Material { diffuseColor " << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
           << "    DrawStyle { lineWidth " << drawStyle.lineWidth << "} \n"
           << "    Coordinate3 { \n"
           << "      point [ "
           <<        triangle.getPoint1().x << " " << triangle.getPoint1().y << " " << triangle.getPoint1().z << ","
           <<        triangle.getPoint2().x << " " << triangle.getPoint2().y << " " << triangle.getPoint2().z << ","
           <<        triangle.getPoint3().x << " " << triangle.getPoint3().y << " " << triangle.getPoint3().z
           << "] \n"
           << "    } \n"
           << "    IndexedLineSet { coordIndex[ 0, 1, 2, 0, -1 ] } \n"
           << fs << '\n'
           << "  } \n";
}

void InventorBuilder::addSinglePlane(const Vector3f& base, const Vector3f& eX, const Vector3f& eY,
                                     float length, float width, DrawStyle drawStyle, const ColorRGB& rgb)
{
    Vector3f pt0 = base;
    Vector3f pt1 = base + length * eX;
    Vector3f pt2 = base + length * eX + width * eY;
    Vector3f pt3 = base + width * eY;
    std::string fs = "";
    if (drawStyle.style == DrawStyle::Style::Filled) {
        fs = "    FaceSet { } ";
    }

    result << "  Separator { \n"
           << "    Material { diffuseColor " << rgb.red() << " "<< rgb.green() << " " << rgb.blue() << "} \n"
           << "    DrawStyle { lineWidth " << drawStyle.lineWidth << "} \n"
           << "    Coordinate3 { \n"
           << "      point [ "
           <<        pt0.x << " " << pt0.y << " " << pt0.z << ","
           <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
           <<        pt2.x << " " << pt2.y << " " << pt2.z << ","
           <<        pt3.x << " " << pt3.y << " " << pt3.z
           << "] \n"
           << "    } \n"
           << "    IndexedLineSet { coordIndex[ 0, 1, 2, 3, 0, -1 ] } \n"
           << fs << '\n'
           << "  } \n";
}

/**
 * The number of control points must be numUControlPoints * numVControlPoints.
 * The order in u or v direction of the NURBS surface is implicitly given by
 * number of elements in uKnots - numUControlPoints or
 * number of elements in vKnots - numVControlPoints.
 */
void InventorBuilder::addNurbsSurface(const std::vector<Base::Vector3f>& controlPoints,
                                      int numUControlPoints, int numVControlPoints,
                                      const std::vector<float>& uKnots,
                                      const std::vector<float>& vKnots)
{
    result << "  Separator { \n"
           << "    Coordinate3 { \n"
           << "      point [ ";
    for (std::vector<Base::Vector3f>::const_iterator it =
        controlPoints.begin(); it != controlPoints.end(); ++it) {
        if (it != controlPoints.begin())
            result << ",\n          ";
        result << it->x << " " << it->y << " " << it->z;
    }

    result << " ]\n"
           << "    }\n";
    result << "    NurbsSurface { \n"
           << "      numUControlPoints " << numUControlPoints << '\n'
           << "      numVControlPoints " << numVControlPoints << '\n'
           << "      uKnotVector [ ";
    int index = 0;
    for (std::vector<float>::const_iterator it = uKnots.begin(); it != uKnots.end(); ++it) {
        result << *it;
        index++;
        if ((it+1) < uKnots.end()) {
            if (index % 4 == 0)
                result << ",\n          ";
            else
                result << ", ";
        }
    }
    result << " ]\n"
           << "      vKnotVector [ ";
    for (std::vector<float>::const_iterator it = vKnots.begin(); it != vKnots.end(); ++it) {
        result << *it;
        index++;
        if ((it+1) < vKnots.end()) {
            if (index % 4 == 0)
                result << ",\n          ";
            else
                result << ", ";
        }
    }
    result << " ]\n"
           << "    }\n"
           << "  }\n";
}

void InventorBuilder::addCone(float bottomRadius, float height)
{
    result << indent << "  Cone { bottomRadius " << bottomRadius << " height " << height << "} \n";
}

void InventorBuilder::addCylinder(float radius, float height)
{
    result << indent << "Cylinder {\n"
           << indent << "  radius " << radius << "\n"
           << indent << "  height " << height << "\n"
           << indent << "  parts (SIDES | TOP | BOTTOM)\n"
           << indent << "}\n";
}

void InventorBuilder::addSphere(float radius)
{
    result << indent << "Sphere {\n"
           << indent << "  radius " << radius << "\n"
           << indent << "}\n";
}

void InventorBuilder::addBoundingBox(const Vector3f& pt1, const Vector3f& pt2, DrawStyle drawStyle, const ColorRGB& rgb)
{
    Base::Vector3f pt[8];
    pt[0].Set(pt1.x, pt1.y, pt1.z);
    pt[1].Set(pt1.x, pt1.y, pt2.z);
    pt[2].Set(pt1.x, pt2.y, pt1.z);
    pt[3].Set(pt1.x, pt2.y, pt2.z);
    pt[4].Set(pt2.x, pt1.y, pt1.z);
    pt[5].Set(pt2.x, pt1.y, pt2.z);
    pt[6].Set(pt2.x, pt2.y, pt1.z);
    pt[7].Set(pt2.x, pt2.y, pt2.z);

    result << "  Separator { \n"
           << "    Material { diffuseColor " << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
           << "    DrawStyle { lineWidth " << drawStyle.lineWidth << "} \n"
           << "    Coordinate3 { \n"
           << "      point [ "
           << "        " << pt[0].x << " " << pt[0].y << " " << pt[0].z << ",\n"
           << "        " << pt[1].x << " " << pt[1].y << " " << pt[1].z << ",\n"
           << "        " << pt[2].x << " " << pt[2].y << " " << pt[2].z << ",\n"
           << "        " << pt[3].x << " " << pt[3].y << " " << pt[3].z << ",\n"
           << "        " << pt[4].x << " " << pt[4].y << " " << pt[4].z << ",\n"
           << "        " << pt[5].x << " " << pt[5].y << " " << pt[5].z << ",\n"
           << "        " << pt[6].x << " " << pt[6].y << " " << pt[6].z << ",\n"
           << "        " << pt[7].x << " " << pt[7].y << " " << pt[7].z
           << "] \n"
           << "    } \n"
           << "    IndexedLineSet { coordIndex[ 0, 2, 6, 4, 0, -1\n"
              "        1, 5, 7, 3, 1, -1,\n"
              "        5, 4, 6, 7, 5, -1,\n"
              "        7, 6, 2, 3, 7, -1,\n"
              "        3, 2, 0, 1, 3, -1,\n"
              "        5, 1, 0, 4, 5, -1 ] } \n"
           << "  } \n";
}

void InventorBuilder::addTransformation(const Matrix4D& transform)
{
    Base::Placement placement;
    placement.fromMatrix(transform);
    addTransformation(placement);
}

void InventorBuilder::addTransformation(const Base::Placement& transform)
{
    Base::Vector3d translation = transform.getPosition();
    Base::Vector3d rotationaxis;
    double angle{};
    transform.getRotation().getValue(rotationaxis, angle);

    result << indent << "Transform {\n";
    result << indent << "  translation "
         << translation.x << " " << translation.y << " " << translation.z << '\n';
    result << indent << "  rotation "
         << rotationaxis.x << " " << rotationaxis.y << " " << rotationaxis.z
         << " " << angle << '\n';
    result << indent <<  "}" << '\n';
}

// -----------------------------------------------------------------------------

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Builder3D::Builder3D()
  : InventorBuilder(result)
{
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Builder3D::~Builder3D() = default;

void Builder3D::clear ()
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
    if (obs){
        obs->SendLog(result.str().c_str(), Base::LogStyle::Log);
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
    Base::ofstream  file(fi);
    if (!file) {
        throw FileException("Cannot open file");
    }

    file << result.str();
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
        std::string::size_type open = str.find("[");
        if (point != std::string::npos && open > point) {
            str = str.substr(open);
            found = true;
            break;
        }
    }

    if (!found)
        return {};

    do {
        boost::char_separator<char> sep(" ,");
        boost::tokenizer<boost::char_separator<char> > tokens(str, sep);
        std::vector<std::string> token_results;
        token_results.assign(tokens.begin(),tokens.end());

        for (const auto& it : token_results) {
            try {
                T value = boost::lexical_cast<T>(it);
                fieldValues.emplace_back(value);
            }
            catch (const boost::bad_lexical_cast&) {
            }
        }

        // search for ']' to finish the reading
        if (str.find("]") != std::string::npos)
            break;
    }
    while (std::getline(inp, str));

    return fieldValues;
}

std::vector<Vector3f> InventorLoader::convert(const std::vector<float>& data) const
{
    if (data.size() % 3 != 0)
        throw std::string("Reading failed");

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
            faces.emplace_back(coordIndex, coordIndex+1, coordIndex+2);
        }
        else if (it == 4) {
            faces.emplace_back(coordIndex, coordIndex+1, coordIndex+2);
            faces.emplace_back(coordIndex, coordIndex+2, coordIndex+3);
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

std::vector<InventorLoader::Face> InventorLoader::convert(const std::vector<std::vector<int32_t>>& coordIndex) const
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
    if (!inp || inp.bad())
        return false;

    std::string line;

    // Verify it's an Inventor 2.1 file
    std::getline(inp, line);
    if (line.find("#Inventor V2.1 ascii") == std::string::npos)
        return false;

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
    int32_t value{static_cast<int32_t>(points.size())};
    auto inRange = [value](const Face& f) {
        if (f.p1 < 0 || f.p1 >= value)
            return false;
        if (f.p2 < 0 || f.p2 >= value)
            return false;
        if (f.p3 < 0 || f.p3 >= value)
            return false;
        return true;
    };
    for (auto it : faces) {
        if (!inRange(it))
            return false;
    }

    return true;
}

namespace Base {
BaseExport Vector3f to_vector(std::string str)
{
    std::string_view view = str;
    if (!boost::starts_with(view, "(") || !boost::ends_with(str, ")"))
        throw std::runtime_error("string is not a tuple");

    view.remove_prefix(1);
    view.remove_suffix(1);

    str = view;

    boost::char_separator<char> sep(" ,");
    boost::tokenizer<boost::char_separator<char> > tokens(str, sep);
    std::vector<std::string> token_results;
    token_results.assign(tokens.begin(), tokens.end());

    if (token_results.size() != 3)
        throw std::runtime_error("not a tuple of three floats");

    Base::Vector3f vec;
    vec.x = boost::lexical_cast<float>(token_results.at(0));
    vec.y = boost::lexical_cast<float>(token_results.at(1));
    vec.z = boost::lexical_cast<float>(token_results.at(2));

    return vec;
}

}
