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


//**************************************************************************
// Construction/Destruction

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

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Builder3D::Builder3D()
  : bStartEndOpen(false)
{
    result << "#Inventor V2.1 ascii \n\n";
    result << "Separator { ";
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Builder3D::~Builder3D() = default;


//**************************************************************************
// points handling

/**
 * Starts the definition of point set with the given point size and color.
 * If possible don't make too many startPoints() and endPoints() calls.
 * Try to put all points in one set.
 * @see endPoints()
 * @param pointSize the point size in pixel that are displayed.
 * @param ColorRGB point color.
 */
void Builder3D::startPoints(short pointSize, const ColorRGB& color)
{
    bStartEndOpen = true;
    result << "Separator { ";
    result <<   "Material { ";
    result <<     "diffuseColor " << color.red() << " "<< color.green() << " " << color.blue();
    result <<   "} ";
    result <<   "MaterialBinding { value PER_PART } ";
    result <<   "DrawStyle { pointSize " << pointSize << "} ";
    result <<   "Coordinate3 { ";
    result <<     "point [ ";
}

/// add a point to a point set
void Builder3D::addPoint(const Vector3f &point)
{
    result << point.x << " " << point.y << " " << point.z << ",";
}
/**
 * Ends the point set operations and write the resulting inventor string.
 * @see startPoints()
 */
void Builder3D::endPoints()
{
    result  <<      "] ";
    result  <<     "} ";
    result  <<   "PointSet { } ";
    result  << "} ";
    bStartEndOpen = false;
}

void Builder3D::addSinglePoint(const Base::Vector3f &point, DrawStyle drawStyle, const ColorRGB& color)
{
    // addSinglePoint() not between startXXX() and endXXX() allowed
    assert(!bStartEndOpen);

    result << "Separator { ";
    result <<   "Material { ";
    result <<     "diffuseColor " << color.red() << " "<< color.green() << " "<< color.blue();
    result <<   "} ";
    result <<   "MaterialBinding { value PER_PART } ";
    result <<   "DrawStyle { pointSize " << drawStyle.pointSize << "} ";
    result <<   "Coordinate3 { ";
    result <<     "point [ ";
    result << point.x << " " << point.y << " " << point.z << ",";
    result <<      "] ";
    result <<     "} ";
    result <<   "PointSet { } ";
    result << "} ";
}

//**************************************************************************
// text handling


/**
 * Add a Text with a given position to the 3D set. The origin is the
 * lower leftmost corner.
 * @param point origin of the text
 * @param text the text to display.
 * @param color text color.
 */
void Builder3D::addText(const Base::Vector3f& point, const char * text, const Base::ColorRGB& color)
{
  // addSinglePoint() not between startXXX() and endXXX() allowed
  assert(!bStartEndOpen);

  result << "Separator { "
         <<   "Material { diffuseColor " << color.red() << " "<< color.green() << " "<< color.blue() << "} "
         <<   "Transform { translation " << point.x << " "<< point.y << " "<< point.z << "} "
         <<   "Text2 { string \" " << text << "\" " << "} "
         << "} ";
}

void Builder3D::clear ()
{
    // under gcc stringstream::str() returns a copy not a reference
#if defined(_MSC_VER)
    result.str().clear();
#endif
    result.clear();
}

//**************************************************************************
// line/arrow handling

void Builder3D::addSingleLine(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& color)
{
    std::string pattern = drawStyle.patternAsString();

    result << "Separator { "
           <<   "Material { diffuseColor " << color.red() << " "<< color.green() << " "<< color.blue() << "} "
           <<   "DrawStyle { lineWidth " << drawStyle.lineWidth << " linePattern " << pattern << " } "
           <<   "Coordinate3 { "
           <<     "point [ "
           <<        line.p1.x << " " << line.p1.y << " " << line.p1.z << ","
           <<        line.p2.x << " " << line.p2.y << " " << line.p2.z
           <<     "] "
           <<   "} "
           <<   "LineSet { } "
           << "} ";
}

void Builder3D::addSingleArrow(const Base::Line3f& line, DrawStyle drawStyle, const ColorRGB& color)
{
    float l = line.Length();
    float cl = l / 10.0f;
    float cr = cl / 2.0f;

    Vector3f dir = line.GetDirection();
    dir.Normalize();
    dir.Scale(l-cl, l-cl, l-cl);
    Vector3f pt2s = line.p1 + dir;
    dir.Normalize();
    dir.Scale(l-cl/2.0f, l-cl/2.0f, l-cl/2.0f);
    Vector3f cpt = line.p1 + dir;

    Vector3f rot = Vector3f(0.0f, 1.0f, 0.0f) % dir;
    rot.Normalize();
    float a = Vector3f(0.0f, 1.0f, 0.0f).GetAngle(dir);

    result << "Separator { "
         <<   "Material { diffuseColor " << color.red() << " "<< color.green() << " "<< color.blue() << "} "
         <<   "DrawStyle { lineWidth " << drawStyle.lineWidth << "} "
         <<   "Coordinate3 { "
         <<     "point [ "
         <<        line.p1.x << " " << line.p1.y << " " << line.p1.z << ","
         <<        pt2s.x << " " << pt2s.y << " " << pt2s.z
         <<     "] "
         <<   "} "
         <<   "LineSet { } "
         <<   "Transform { "
         <<     "translation " << cpt.x << " " << cpt.y << " " << cpt.z << " "
         <<     "rotation " << rot.x << " " << rot.y << " " << rot.z << " " << a
         <<   "} "
         <<   "Cone { bottomRadius " << cr << " height " << cl << "} "
         << "} ";

}

//**************************************************************************
// triangle handling

void Builder3D::addSingleTriangle(const Triangle& triangle, DrawStyle drawStyle, const ColorRGB& color)
{
    std::string fs = "";
    if (drawStyle.style == DrawStyle::Style::Filled) {
        fs = "IndexedFaceSet { coordIndex[ 0, 1, 2, -1 ] } ";
    }

    result << "Separator { "
           <<   "Material { diffuseColor " << color.red() << " "<< color.green() << " "<< color.blue() << "} "
           <<   "DrawStyle { lineWidth " << drawStyle.lineWidth << "} "
           <<   "Coordinate3 { "
           <<     "point [ "
           <<        triangle.getPoint1().x << " " << triangle.getPoint1().y << " " << triangle.getPoint1().z << ","
           <<        triangle.getPoint2().x << " " << triangle.getPoint2().y << " " << triangle.getPoint2().z << ","
           <<        triangle.getPoint3().x << " " << triangle.getPoint3().y << " " << triangle.getPoint3().z << ","
           <<     "] "
           <<   "} "
           <<   "LineSet { } "
           <<   fs
           << "} ";
}

void Builder3D::addTransformation(const Base::Matrix4D& transform)
{
    Base::Placement placement;
    placement.fromMatrix(transform);
    addTransformation(placement);
}

void Builder3D::addTransformation(const Base::Placement& transform)
{
    Base::Vector3d translation = transform.getPosition();
    Base::Vector3d rotationaxis;
    double angle{};
    transform.getRotation().getValue(rotationaxis, angle);
    result << "Transform {";
    result << "  translation " << translation.x << " " << translation.y << " " << translation.z;
    result << "  rotation " << rotationaxis.x << " " << rotationaxis.y << " " << rotationaxis.z << " " << angle;
    result << "}";
}

//**************************************************************************
// output handling

/**
 * Save the resulting inventor 3D representation to the Console().Log() facility.
 * In DEBUG mode the Gui (if running) will trigger on that and show the representation in
 * the active Viewer/Document. It shows only one representation on time. If you need to
 * show more then one representation use saveToFile() instead.
 * @see saveToFile()
 */
void Builder3D::saveToLog()
{
    result <<   "} ";
    // Note: The string can become very long, so that ConsoleSingelton::Log() will internally
    // truncate the string which causes Inventor to fail to interpret the truncated string.
    // So, we send the string directly to the observer that handles the Inventor stuff.
    //Console().Log("Vdbg: %s \n",result.str().c_str());
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
    result <<   "} ";
    Base::FileInfo fi(FileName);
    Base::ofstream  file(fi);
    if (!file)
        throw FileException("Builder3D::saveToFile(): Can not open file...");

    file << "#Inventor V2.1 ascii \n";
    file << result.str();
}

// -----------------------------------------------------------------------------

InventorBuilder::InventorBuilder(std::ostream& output)
  : result(output)
  , indent(0)
{
    result << "#Inventor V2.1 ascii \n\n";
    beginSeparator();
}

InventorBuilder:: ~InventorBuilder()
{
    close();
}

void InventorBuilder::close()
{
    if (indent > 0) {
        indent = 0;
        endSeparator();
    }
}

void InventorBuilder::beginSeparator()
{
    result << Base::blanks(indent) << "Separator { \n";
    indent += 2;
}

void InventorBuilder::endSeparator()
{
    indent-=2;
    result << Base::blanks(indent) << "}\n";
}

void InventorBuilder::addInfo(const char* text)
{
    result << Base::blanks(indent) << "Info { \n";
    result << Base::blanks(indent) << "  string \"" << text << "\"\n";
    result << Base::blanks(indent) << "} \n";
}

void InventorBuilder::addLabel(const char* text)
{
    result << Base::blanks(indent) << "Label { \n";
    result << Base::blanks(indent) << "  label \"" << text << "\"\n";
    result << Base::blanks(indent) << "} \n";
}

void InventorBuilder::addBaseColor(const ColorRGB& rgb)
{
    result << Base::blanks(indent) << "BaseColor { \n";
    result << Base::blanks(indent) << "  rgb "
           << rgb.red() << " " << rgb.green() << " " << rgb.blue() << '\n';
    result << Base::blanks(indent) << "} \n";
}

void InventorBuilder::addMaterial(const ColorRGB& rgb, float transparency)
{
    result << Base::blanks(indent) << "Material { \n";
    result << Base::blanks(indent) << "  diffuseColor "
           << rgb.red() << " " << rgb.green() << " " << rgb.blue() << '\n';
    if (transparency > 0)
        result << Base::blanks(indent) << "  transparency " << transparency << '\n';
    result << Base::blanks(indent) << "} \n";
}

void InventorBuilder::beginMaterial()
{
    result << Base::blanks(indent) << "Material { \n";
    indent += 2;
    result << Base::blanks(indent) << "diffuseColor [\n";
    indent += 2;
}

void InventorBuilder::endMaterial()
{
    indent -= 2;
    result << Base::blanks(indent) << "]\n";
    indent -= 2;
    result << Base::blanks(indent) << "}\n";
}

void InventorBuilder::addColor(const ColorRGB& rgb)
{
    result << rgb.red() << " " << rgb.green() << " " << rgb.blue() << '\n';
}

void InventorBuilder::addMaterialBinding(const char* bind)
{
    result << Base::blanks(indent) << "MaterialBinding { value "
           << bind << " } \n";
}

void InventorBuilder::addDrawStyle(DrawStyle drawStyle)
{
    result << Base::blanks(indent) << "DrawStyle {\n"
           << Base::blanks(indent) << "  style " << drawStyle.styleAsString() << '\n'
           << Base::blanks(indent) << "  pointSize " << drawStyle.pointSize << '\n'
           << Base::blanks(indent) << "  lineWidth " << drawStyle.lineWidth << '\n'
           << Base::blanks(indent) << "  linePattern " << drawStyle.linePattern << '\n'
           << Base::blanks(indent) << "}\n";
}

void InventorBuilder::addShapeHints(float creaseAngle)
{
    result << Base::blanks(indent) << "ShapeHints {\n"
           << Base::blanks(indent) << "  creaseAngle " << creaseAngle << '\n'
           << Base::blanks(indent) << "}\n";
}

void InventorBuilder::addPolygonOffset(PolygonOffset polygonOffset)
{
    result << Base::blanks(indent) << "PolygonOffset {\n"
           << Base::blanks(indent) << "  factor " << polygonOffset.factor << '\n'
           << Base::blanks(indent) << "  units " << polygonOffset.units << '\n'
           << Base::blanks(indent) << "  styles " << polygonOffset.styleAsString() << '\n'
           << Base::blanks(indent) << "  on " << (polygonOffset.on ? "TRUE" : "FALSE") << '\n'
           << Base::blanks(indent) << "}\n";
}

//**************************************************************************
// points handling

/**
 * Starts the definition of point set.
 * If possible don't make too many beginPoints() and endPoints() calls.
 * Try to put all points in one set.
 * @see startPoints()
 * @see endPoints()
 */
void InventorBuilder::beginPoints()
{
    result << Base::blanks(indent) << "Coordinate3 { \n";
    indent += 2;
    result << Base::blanks(indent) << "point [ \n";
    indent += 2;
}

/// insert a point in a point set
void InventorBuilder::addPoint(const Vector3f& pnt)
{
    result << Base::blanks(indent) << pnt.x << " " << pnt.y << " " << pnt.z << ",\n";
}

void InventorBuilder::addPoints(const std::vector<Vector3f>& points)
{
    for (const auto& pnt : points) {
        addPoint(pnt);
    }
}

/**
 * Ends the point set operations and write the resulting inventor string.
 * @see startPoints()
 */
void InventorBuilder::endPoints()
{
    indent -= 2;
    result << Base::blanks(indent) << "]\n";
    indent -= 2;
    result << Base::blanks(indent) << "}\n";
}

/**
 * Adds an SoPointSet node after creating an SoCordinate3 node with
 * beginPoints() and endPoints().
 * @see startPoints()
 * @see beginPoints()
 * @see endPoints()
 */
void InventorBuilder::addPointSet()
{
    result << Base::blanks(indent) << "PointSet { } \n";
}

/**
 * Adds a SoLineSet node after creating a SoCordinate3 node with
 * beginPoints() and endPoints().
 * @see startPoints()
 * @see beginPoints()
 * @see endPoints()
 */
void InventorBuilder::addLineSet()
{
    result << Base::blanks(indent) << "LineSet { } \n";
}

//**************************************************************************
// text handling


/**
 * Add a Text with a given position to the 3D set. The origin is the
 * lower leftmost corner.
 * @param pos_x,pos_y,pos_z origin of the text
 * @param text the text to display.
 * @param color text color.
 */
void InventorBuilder::addText(const Vector3f& pnt, const char * text, const ColorRGB& rgb)
{
  result << Base::blanks(indent) << "Separator { \n"
         << Base::blanks(indent) << "  Material { diffuseColor "
         << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
         << Base::blanks(indent) << "  Transform { translation "
         << pnt.x << " "<< pnt.y << " "<< pnt.z << "} \n"
         << Base::blanks(indent) << "  Text2 { string \" " << text << "\" " << "} \n"
         << Base::blanks(indent) << "}\n";

}

//**************************************************************************
// line/arrow handling

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
    float l = line.Length();
    float cl = l / 10.0f;
    float cr = cl / 2.0f;

    Vector3f dir = line.GetDirection();
    dir.Normalize();
    dir.Scale(l-cl, l-cl, l-cl);
    Vector3f pt2s = line.p1 + dir;
    dir.Normalize();
    dir.Scale(l-cl/2.0f, l-cl/2.0f, l-cl/2.0f);
    Vector3f cpt = line.p1 + dir;

    Vector3f rot = Vector3f(0.0f, 1.0f, 0.0f) % dir;
    rot.Normalize();
    float a = Vector3f(0.0f, 1.0f, 0.0f).GetAngle(dir);

    result << Base::blanks(indent) << "Separator { \n"
           << Base::blanks(indent) << "  Material { diffuseColor "
           << rgb.red() << " "<< rgb.green() << " "<< rgb.blue() << "} \n"
           << Base::blanks(indent) << "  DrawStyle { lineWidth "
           << drawStyle.lineWidth << "} \n"
           << Base::blanks(indent) << "  Coordinate3 { \n"
           << Base::blanks(indent) << "    point [ "
           <<        line.p1.x << " " << line.p1.y << " " << line.p1.z << ","
           <<        pt2s.x << " " << pt2s.y << " " << pt2s.z
           << " ] \n"
           << Base::blanks(indent) << "  } \n"
           << Base::blanks(indent) << "  LineSet { } \n"
           << Base::blanks(indent) << "  Transform { \n"
           << Base::blanks(indent) << "    translation "
           << cpt.x << " " << cpt.y << " " << cpt.z << " \n"
           << Base::blanks(indent) << "    rotation "
           << rot.x << " " << rot.y << " " << rot.z << " " << a << '\n'
           << Base::blanks(indent) << "  } \n"
           << Base::blanks(indent) << "  Cone { bottomRadius " << cr << " height " << cl << "} \n"
           << Base::blanks(indent) << "} \n";
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

//**************************************************************************
// triangle handling

void InventorBuilder::addIndexedFaceSet(const std::vector<int>& indices)
{
    if (indices.size() < 4)
        return;

    result << Base::blanks(indent) << "IndexedFaceSet { \n"
           << Base::blanks(indent) << "  coordIndex [ \n";

    indent += 4;
    std::vector<int>::const_iterator it_last_f = indices.end()-1;
    int index=0;
    for (std::vector<int>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
        if (index % 8 == 0)
            result << Base::blanks(indent);
        if (it != it_last_f)
            result << *it << ", ";
        else
            result << *it << " ] \n";
        if (++index % 8 == 0)
            result << '\n';
    }
    indent -= 4;

    result << Base::blanks(indent) << "} \n";
}

void InventorBuilder::addFaceSet(const std::vector<int>& vertices)
{
    result << Base::blanks(indent) << "FaceSet { \n"
           << Base::blanks(indent) << "  numVertices [ \n";

    indent += 4;
    std::vector<int>::const_iterator it_last_f = vertices.end()-1;
    int index=0;
    for (std::vector<int>::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
        if (index % 8 == 0)
            result << Base::blanks(indent);
        if (it != it_last_f)
            result << *it << ", ";
        else
            result << *it << " ] \n";
        if (++index % 8 == 0)
            result << '\n';
    }
    indent -= 4;

    result << Base::blanks(indent) << "} \n";
}

void InventorBuilder::beginNormal()
{
    result << Base::blanks(indent) << "Normal { \n";
    indent += 2;
    result << Base::blanks(indent) << "vector [ \n";
    indent += 2;
}

void InventorBuilder::endNormal()
{
    indent -= 2;
    result << Base::blanks(indent) << "]\n";
    indent -= 2;
    result << Base::blanks(indent) << "}\n";
}

void InventorBuilder::addNormalBinding(const char* binding)
{
    result << Base::blanks(indent) << "NormalBinding {\n"
           << Base::blanks(indent) << "  value " << binding << '\n'
           << Base::blanks(indent) << "}\n";
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

void InventorBuilder::addCylinder(float radius, float height)
{
    result << Base::blanks(indent) << "Cylinder {\n"
           << Base::blanks(indent) << "  radius " << radius << "\n"
           << Base::blanks(indent) << "  height " << height << "\n"
           << Base::blanks(indent) << "  parts (SIDES | TOP | BOTTOM)\n"
           << Base::blanks(indent) << "}\n";
}

void InventorBuilder::addSphere(float radius)
{
    result << Base::blanks(indent) << "Sphere {\n"
           << Base::blanks(indent) << "  radius " << radius << "\n"
           << Base::blanks(indent) << "}\n";
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

    result << Base::blanks(indent) << "Transform {\n";
    result << Base::blanks(indent) << "  translation "
         << translation.x << " " << translation.y << " " << translation.z << '\n';
    result << Base::blanks(indent) << "  rotation "
         << rotationaxis.x << " " << rotationaxis.y << " " << rotationaxis.z
         << " " << angle << '\n';
    result << Base::blanks(indent) <<  "}" << '\n';
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
