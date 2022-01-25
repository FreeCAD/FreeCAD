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
# include <cassert>
# include <string>
#endif

#include <cstdlib>

/// FreeCAD #includes sorted by Base,App,Gui......
#include "Builder3D.h"
#include "Exception.h"
#include "Vector3D.h"
#include "Matrix.h"
#include "Console.h"
#include "Tools.h"

using namespace Base;


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Builder3D::Builder3D()
:bStartEndOpen(false)
{
  result << "#Inventor V2.1 ascii " << std::endl << std::endl;
  result << "Separator { ";
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Builder3D::~Builder3D()
{
}


//**************************************************************************
// points handling

/**
 * Starts the definition of point set with the given point size and color.
 * If possible don't make too many startPoints() and endPoints() calls.
 * Try to put all points in one set.
 * @see endPoints()
 * @param pointSize the point size in pixel that are displayed.
 * @param color_r red part of the point color (0.0 - 1.0).
 * @param color_g green part of the point color (0.0 - 1.0).
 * @param color_b blue part of the point color (0.0 - 1.0).
 */
void Builder3D::startPoints(short pointSize, float color_r,float color_g,float color_b)
{
  bStartEndOpen = true;
  result << "Separator { ";
  result <<   "Material { ";
  result <<     "diffuseColor " << color_r << " "<< color_g << " "<< color_b  ;
  result <<   "} ";
  result <<   "MaterialBinding { value PER_PART } ";
  result <<   "DrawStyle { pointSize " << pointSize << "} ";
  result <<   "Coordinate3 { ";
  result <<     "point [ ";
}

/// insert a point in a point set
void Builder3D::addPoint(float x, float y, float z)
{
  result << x << " " << y << " " << z << ",";
}


/// add a vector to a point set
void Builder3D::addPoint(const Vector3f &vec)
{
  addPoint(vec.x,vec.y,vec.z);
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

void Builder3D::addSinglePoint(float x, float y, float z,short pointSize, float color_r,float color_g,float color_b)
{
  // addSinglePoint() not between startXXX() and endXXX() allowed
  assert( bStartEndOpen == false );

  result << "Separator { ";
  result <<   "Material { ";
  result <<     "diffuseColor " << color_r << " "<< color_g << " "<< color_b  ;
  result <<   "} ";
  result <<   "MaterialBinding { value PER_PART } ";
  result <<   "DrawStyle { pointSize " << pointSize << "} ";
  result <<   "Coordinate3 { ";
  result <<     "point [ ";
  result << x << " " << y << " " << z << ",";
  result <<      "] ";
  result <<     "} ";
  result <<   "PointSet { } ";
  result << "} ";

}

void Builder3D::addSinglePoint(const Base::Vector3f &vec, short pointSize, float color_r,float color_g,float color_b)
{
  addSinglePoint(vec.x, vec.y , vec.z, pointSize, color_r, color_g, color_b);
}

//**************************************************************************
// text handling


/**
 * Add a Text with a given position to the 3D set. The origin is the
 * lower leftmost corner.
 * @param pos_x,pos_y,pos_z origin of the text
 * @param text the text to display.
 * @param color_r red part of the text color (0.0 - 1.0).
 * @param color_g green part of the text color (0.0 - 1.0).
 * @param color_b blue part of the text color (0.0 - 1.0).
 */
void Builder3D::addText(float pos_x, float pos_y , float pos_z,const char * text, float color_r,float color_g,float color_b)
{
  // addSinglePoint() not between startXXX() and endXXX() allowed
  assert( bStartEndOpen == false );

  result << "Separator { "
         <<   "Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "
         <<   "Transform { translation " << pos_x << " "<< pos_y << " "<< pos_z << "} "
         <<   "Text2 { string \" " << text << "\" " << "} "
         << "} ";

}

void Builder3D::addText(const Base::Vector3f &vec,const char * text, float color_r,float color_g,float color_b)
{
  addText(vec.x, vec.y , vec.z,text, color_r,color_g,color_b);
}

void Builder3D::clear ()
{
  // Under VC6 string::clear() doesn't exist, under gcc stringstream::str() returns a copy not a reference
#if defined(_MSC_VER) && _MSC_VER >= 1400
  result.str().clear();
#endif
  result.clear();
}

//**************************************************************************
// line/arrow handling

void Builder3D::addSingleLine(const Vector3f& pt1, const Vector3f& pt2, short lineSize, float color_r,float color_g,float color_b, unsigned short linePattern)
{
  char lp[20];
  sprintf(lp, "0x%x", linePattern);
  //char lp[20] = "0x";
  //itoa(linePattern, buf, 16);
  //strcat(lp, buf);

  result << "Separator { "
         <<   "Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "
         <<   "DrawStyle { lineWidth " << lineSize << " linePattern " << lp << " } "
         <<   "Coordinate3 { "
         <<     "point [ "
         <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
         <<        pt2.x << " " << pt2.y << " " << pt2.z
         <<     "] "
         <<   "} "
         <<   "LineSet { } "
         << "} ";
}

void Builder3D::addSingleArrow(const Vector3f& pt1, const Vector3f& pt2, short lineSize, float color_r,float color_g,float color_b, unsigned short /*linePattern*/)
{
    float l = (pt2 - pt1).Length();
    float cl = l / 10.0f;
    float cr = cl / 2.0f;

    Vector3f dir = pt2 - pt1;
    dir.Normalize();
    dir.Scale(l-cl, l-cl, l-cl);
    Vector3f pt2s = pt1 + dir;
    dir.Normalize();
    dir.Scale(l-cl/2.0f, l-cl/2.0f, l-cl/2.0f);
    Vector3f cpt = pt1 + dir;

    Vector3f rot = Vector3f(0.0f, 1.0f, 0.0f) % dir;
    rot.Normalize();
    float a = Vector3f(0.0f, 1.0f, 0.0f).GetAngle(dir);

    result << "Separator { "
         <<   "Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "
         <<   "DrawStyle { lineWidth " << lineSize << "} "
         <<   "Coordinate3 { "
         <<     "point [ "
         <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
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

void Builder3D::addSingleTriangle(const Vector3f& pt0, const Vector3f& pt1, const Vector3f& pt2, bool filled, short lineSize, float color_r, float color_g, float color_b)
{
  std::string fs = "";
  if (filled)
  {
    fs = "IndexedFaceSet { coordIndex[ 0, 1, 2, -1 ] } ";
  }

    result << "Separator { "
         <<   "Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "
         <<   "DrawStyle { lineWidth " << lineSize << "} "
         <<   "Coordinate3 { "
         <<     "point [ "
         <<        pt0.x << " " << pt0.y << " " << pt0.z << ","
         <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
         <<        pt2.x << " " << pt2.y << " " << pt2.z << ","
         <<     "] "
         <<   "} "
         <<   "LineSet { } "
         <<   fs
         << "} ";

}

void Builder3D::addTransformation(const Base::Matrix4D& transform)
{
  Base::Vector3f cAxis, cBase;
  float fAngle = 0.0f, fTranslation = 0.0f;
  transform.toAxisAngle(cBase, cAxis,fAngle,fTranslation);
  cBase.x = static_cast<float>(transform[0][3]);
  cBase.y = static_cast<float>(transform[1][3]);
  cBase.z = static_cast<float>(transform[2][3]);
  addTransformation(cBase,cAxis,fAngle);
}

void Builder3D::addTransformation(const Base::Vector3f& translation, const Base::Vector3f& rotationaxis, float fAngle)
{
  result << "Transform {";
  result << "  translation " << translation.x << " " << translation.y << " " << translation.z;
  result << "  rotation " << rotationaxis.x << " " << rotationaxis.y << " " << rotationaxis.z << " " << fAngle;
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
    if (obs != nullptr){
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
  std::ofstream  file(FileName);
  if(!file)
    throw FileException("Builder3D::saveToFile(): Can not open file...");

  file << "#Inventor V2.1 ascii " << std::endl;
  file << result.str();
}

// -----------------------------------------------------------------------------

InventorBuilder::InventorBuilder(std::ostream& output)
  : result(output)
  , indent(0)
{
    result << "#Inventor V2.1 ascii " << std::endl << std::endl;
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
    result << Base::blanks(indent) << "Separator { " << std::endl;
    indent+=2;
}

void InventorBuilder::endSeparator()
{
    indent-=2;
    result << Base::blanks(indent) << "}" << std::endl;
}

void InventorBuilder::addInfo(const char* text)
{
    result << Base::blanks(indent) << "Info { " << std::endl;
    result << Base::blanks(indent) << "  string \"" << text << "\"" << std::endl;
    result << Base::blanks(indent) << "} " << std::endl;
}

void InventorBuilder::addLabel(const char* text)
{
    result << Base::blanks(indent) << "Label { " << std::endl;
    result << Base::blanks(indent) << "  label \"" << text << "\"" << std::endl;
    result << Base::blanks(indent) << "} " << std::endl;
}

void InventorBuilder::addBaseColor(float color_r,float color_g,float color_b)
{
    result << Base::blanks(indent) << "BaseColor { " << std::endl;
    result << Base::blanks(indent) << "  rgb "
           << color_r << " "<< color_g << " "<< color_b << std::endl;
    result << Base::blanks(indent) << "} " << std::endl;
}

void InventorBuilder::addMaterial(float color_r,float color_g,float color_b,float color_a)
{
    result << Base::blanks(indent) << "Material { " << std::endl;
    result << Base::blanks(indent) << "  diffuseColor "
           << color_r << " "<< color_g << " "<< color_b << std::endl;
    if (color_a > 0)
        result << Base::blanks(indent) << "  transparency " << color_a << std::endl;
    result << Base::blanks(indent) << "} " << std::endl;
}

void InventorBuilder::beginMaterial()
{
    result << Base::blanks(indent) << "Material { " << std::endl;
    indent +=2;
    result << Base::blanks(indent) << "diffuseColor [" << std::endl;
    indent +=2;
}

void InventorBuilder::endMaterial()
{
    indent -=2;
    result << Base::blanks(indent) << "]" << std::endl;
    indent -=2;
    result << Base::blanks(indent) << "}" << std::endl;
}

void InventorBuilder::addColor(float color_r,float color_g,float color_b)
{
    result << color_r << " " << color_g << " " << color_b << std::endl;
}

void InventorBuilder::addMaterialBinding(const char* bind)
{
    result << Base::blanks(indent) << "MaterialBinding { value "
           << bind << " } " << std::endl;
}

void InventorBuilder::addDrawStyle(short pointSize, short lineWidth, unsigned short linePattern, const char* style)
{
    result << Base::blanks(indent) << "DrawStyle {" << std::endl
           << Base::blanks(indent) << "  style " << style << std::endl
           << Base::blanks(indent) << "  pointSize " << pointSize << std::endl
           << Base::blanks(indent) << "  lineWidth " << lineWidth << std::endl
           << Base::blanks(indent) << "  linePattern " << linePattern << std::endl
           << Base::blanks(indent) << "}" << std::endl;
}

void InventorBuilder::addShapeHints(float crease)
{
    result << Base::blanks(indent) << "ShapeHints {" << std::endl
           << Base::blanks(indent) << "  creaseAngle " << crease << std::endl
           << Base::blanks(indent) << "}" << std::endl;
}

void InventorBuilder::addPolygonOffset(float factor, float units, const char* styles, bool on)
{
    result << Base::blanks(indent) << "PolygonOffset {" << std::endl
           << Base::blanks(indent) << "  factor " << factor << std::endl
           << Base::blanks(indent) << "  units " << units << std::endl
           << Base::blanks(indent) << "  styles " << styles << std::endl
           << Base::blanks(indent) << "  on " << (on ? "TRUE" : "FALSE") << std::endl
           << Base::blanks(indent) << "}" << std::endl;
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
    result << Base::blanks(indent) << "Coordinate3 { " << std::endl;
    indent += 2;
    result << Base::blanks(indent) << "point [ " << std::endl;
    indent += 2;
}

/// insert a point in a point set
void InventorBuilder::addPoint(float x, float y, float z)
{
    result << Base::blanks(indent) << x << " " << y << " " << z << "," << std::endl;
}

/// add a vector to a point set
void InventorBuilder::addPoint(const Vector3f &vec)
{
    addPoint(vec.x,vec.y,vec.z);
}

void InventorBuilder::addPoints(const std::vector<Vector3f> &vec)
{
    for (std::vector<Vector3f>::const_iterator it = vec.begin(); it != vec.end(); ++it)
        addPoint(it->x, it->y, it->z);
}

/**
 * Ends the point set operations and write the resulting inventor string.
 * @see startPoints()
 */
void InventorBuilder::endPoints()
{
    indent -= 2;
    result << Base::blanks(indent) << "]" << std::endl;
    indent -= 2;
    result << Base::blanks(indent) << "}" << std::endl;
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
    result << Base::blanks(indent) << "PointSet { } " << std::endl;
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
    result << Base::blanks(indent) << "LineSet { } " << std::endl;
}

//**************************************************************************
// text handling


/**
 * Add a Text with a given position to the 3D set. The origin is the
 * lower leftmost corner.
 * @param pos_x,pos_y,pos_z origin of the text
 * @param text the text to display.
 * @param color_r red part of the text color (0.0 - 1.0).
 * @param color_g green part of the text color (0.0 - 1.0).
 * @param color_b blue part of the text color (0.0 - 1.0).
 */
void InventorBuilder::addText(float pos_x, float pos_y , float pos_z,const char * text, float color_r,float color_g,float color_b)
{
  result << Base::blanks(indent) << "Separator { "   << std::endl
         << Base::blanks(indent) << "  Material { diffuseColor "
         << color_r << " "<< color_g << " "<< color_b << "} "  << std::endl
         << Base::blanks(indent) << "  Transform { translation "
         << pos_x << " "<< pos_y << " "<< pos_z << "} "  << std::endl
         << Base::blanks(indent) << "  Text2 { string \" " << text << "\" " << "} " << std::endl
         << Base::blanks(indent) << "}" << std::endl;

}

void InventorBuilder::addText(const Vector3f &vec,const char * text, float color_r,float color_g,float color_b)
{
    addText(vec.x, vec.y , vec.z,text, color_r,color_g,color_b);
}

//**************************************************************************
// line/arrow handling

void InventorBuilder::addSingleLine(const Vector3f& pt1, const Vector3f& pt2, short lineSize,
                                    float color_r,float color_g,float color_b, unsigned short linePattern)
{
  char lp[20];
  sprintf(lp, "0x%x", linePattern);
  //char lp[20] = "0x";
  //itoa(linePattern, buf, 16);
  //strcat(lp, buf);

  result << "  Separator { " << std::endl
         << "    Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "  << std::endl
         << "    DrawStyle { lineWidth " << lineSize << " linePattern " << lp << " } " << std::endl
         << "    Coordinate3 { " << std::endl
         << "      point [ "
         <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
         <<        pt2.x << " " << pt2.y << " " << pt2.z
         << " ] " << std::endl
         << "    } " << std::endl
         << "    LineSet { } " << std::endl
         << "  } " << std::endl;
}

void InventorBuilder::addSingleArrow(const Vector3f& pt1, const Vector3f& pt2, short lineSize,
                                     float color_r,float color_g,float color_b, unsigned short /*linePattern*/)
{
    float l = (pt2 - pt1).Length();
    float cl = l / 10.0f;
    float cr = cl / 2.0f;

    Vector3f dir = pt2 - pt1;
    dir.Normalize();
    dir.Scale(l-cl, l-cl, l-cl);
    Vector3f pt2s = pt1 + dir;
    dir.Normalize();
    dir.Scale(l-cl/2.0f, l-cl/2.0f, l-cl/2.0f);
    Vector3f cpt = pt1 + dir;

    Vector3f rot = Vector3f(0.0f, 1.0f, 0.0f) % dir;
    rot.Normalize();
    float a = Vector3f(0.0f, 1.0f, 0.0f).GetAngle(dir);

    result << Base::blanks(indent) << "Separator { " << std::endl
           << Base::blanks(indent) << "  Material { diffuseColor "
           << color_r << " "<< color_g << " "<< color_b << "} "  << std::endl
           << Base::blanks(indent) << "  DrawStyle { lineWidth "
           << lineSize << "} " << std::endl
           << Base::blanks(indent) << "  Coordinate3 { " << std::endl
           << Base::blanks(indent) << "    point [ "
           <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
           <<        pt2s.x << " " << pt2s.y << " " << pt2s.z
           << " ] " << std::endl
           << Base::blanks(indent) << "  } " << std::endl
           << Base::blanks(indent) << "  LineSet { } " << std::endl
           << Base::blanks(indent) << "  Transform { " << std::endl
           << Base::blanks(indent) << "    translation "
           << cpt.x << " " << cpt.y << " " << cpt.z << " " << std::endl
           << Base::blanks(indent) << "    rotation "
           << rot.x << " " << rot.y << " " << rot.z << " " << a << std::endl
           << Base::blanks(indent) << "  } " << std::endl
           << Base::blanks(indent) << "  Cone { bottomRadius " << cr << " height " << cl << "} " << std::endl
           << Base::blanks(indent) << "} " << std::endl;
}

/** Add a line defined by a list of points whereat always a pair (i.e. a point and the following point) builds a line.
 * The size of the list must then be even.
 */
void InventorBuilder::addLineSet(const std::vector<Vector3f>& points, short lineSize,
                                 float color_r,float color_g,float color_b, unsigned short linePattern)
{
  char lp[20];
  sprintf(lp, "0x%x", linePattern);

  result << "  Separator { " << std::endl
         << "    Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "  << std::endl
         << "    DrawStyle { lineWidth " << lineSize << " linePattern " << lp << " } " << std::endl
         << "    Coordinate3 { " << std::endl
         << "      point [ ";
  std::vector<Vector3f>::const_iterator it = points.begin();
  if ( it != points.end() )
  {
    result << it->x << " " << it->y << " " << it->z;
    for ( ++it ; it != points.end(); ++it )
      result << "," << std::endl << "          " << it->x << " " << it->y << " " << it->z;
  }

  result << " ] " << std::endl
         << "    } " << std::endl
         << "    LineSet { " << std::endl
         << "      numVertices [ ";
  /*size_t ct = points.size() / 2;
  if ( ct > 0 )
  {
    result << "2";
    for (size_t i=1; i<ct; i++)
    {
      result << ",";
      if (i%16==0)
        result << std::endl << "          ";
      result << "2";
    }
  }*/
  result << " -1 ";
  result << " ] " << std::endl
         << "    } " << std::endl
         << "  } " << std::endl;
}

//**************************************************************************
// triangle handling

void InventorBuilder::addIndexedFaceSet(const std::vector<int>& indices)
{
    if (indices.size() < 4)
        return;

    result << Base::blanks(indent) << "IndexedFaceSet { " << std::endl
           << Base::blanks(indent) << "  coordIndex [ " << std::endl;

    indent += 4;
    std::vector<int>::const_iterator it_last_f = indices.end()-1;
    int index=0;
    for (std::vector<int>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
        if (index % 8 == 0)
            result << Base::blanks(indent);
        if (it != it_last_f)
            result << *it << ", ";
        else
            result << *it << " ] " << std::endl;
        if (++index % 8 == 0)
            result << std::endl;
    }
    indent -= 4;

    result << Base::blanks(indent) << "} " << std::endl;
}

void InventorBuilder::addFaceSet(const std::vector<int>& vertices)
{
    result << Base::blanks(indent) << "FaceSet { " << std::endl
           << Base::blanks(indent) << "  numVertices [ " << std::endl;

    indent += 4;
    std::vector<int>::const_iterator it_last_f = vertices.end()-1;
    int index=0;
    for (std::vector<int>::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
        if (index % 8 == 0)
            result << Base::blanks(indent);
        if (it != it_last_f)
            result << *it << ", ";
        else
            result << *it << " ] " << std::endl;
        if (++index % 8 == 0)
            result << std::endl;
    }
    indent -= 4;

    result << Base::blanks(indent) << "} " << std::endl;
}

void InventorBuilder::beginNormal()
{
    result << Base::blanks(indent) << "Normal { " << std::endl;
    indent += 2;
    result << Base::blanks(indent) << "vector [ " << std::endl;
    indent += 2;
}

void InventorBuilder::endNormal()
{
    indent -= 2;
    result << Base::blanks(indent) << "]" << std::endl;
    indent -= 2;
    result << Base::blanks(indent) << "}" << std::endl;
}

void InventorBuilder::addNormalBinding(const char* binding)
{
    result << Base::blanks(indent) << "NormalBinding {" << std::endl
           << Base::blanks(indent) << "  value " << binding << std::endl
           << Base::blanks(indent) << "}" << std::endl;
}

void InventorBuilder::addSingleTriangle(const Vector3f& pt0, const Vector3f& pt1, const Vector3f& pt2,
                                        bool filled, short lineSize, float color_r, float color_g, float color_b)
{
  std::string fs = "";
  if (filled)
  {
    fs = "    FaceSet { } ";
  }

    result << "  Separator { " << std::endl
           << "    Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "  << std::endl
           << "    DrawStyle { lineWidth " << lineSize << "} " << std::endl
           << "    Coordinate3 { " << std::endl
           << "      point [ "
           <<        pt0.x << " " << pt0.y << " " << pt0.z << ","
           <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
           <<        pt2.x << " " << pt2.y << " " << pt2.z
           << "] " << std::endl
           << "    } " << std::endl
           << "    IndexedLineSet { coordIndex[ 0, 1, 2, 0, -1 ] } " << std::endl
           << fs << std::endl
           << "  } " << std::endl;
}

void InventorBuilder::addSinglePlane(const Vector3f& base, const Vector3f& eX, const Vector3f& eY,
                                     float length, float width, bool filled, short lineSize,
                                     float color_r,float color_g,float color_b)
{
    Vector3f pt0 = base;
    Vector3f pt1 = base + length * eX;
    Vector3f pt2 = base + length * eX + width * eY;
    Vector3f pt3 = base + width * eY;
    std::string fs = "";
    if (filled)
    {
        fs = "    FaceSet { } ";
    }

    result << "  Separator { " << std::endl
           << "    Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "  << std::endl
           << "    DrawStyle { lineWidth " << lineSize << "} " << std::endl
           << "    Coordinate3 { " << std::endl
           << "      point [ "
           <<        pt0.x << " " << pt0.y << " " << pt0.z << ","
           <<        pt1.x << " " << pt1.y << " " << pt1.z << ","
           <<        pt2.x << " " << pt2.y << " " << pt2.z << ","
           <<        pt3.x << " " << pt3.y << " " << pt3.z
           << "] " << std::endl
           << "    } " << std::endl
           << "    IndexedLineSet { coordIndex[ 0, 1, 2, 3, 0, -1 ] } " << std::endl
           << fs << std::endl
           << "  } " << std::endl;
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
    result << "  Separator { " << std::endl
           << "    Coordinate3 { " << std::endl
           << "      point [ ";
    for (std::vector<Base::Vector3f>::const_iterator it =
        controlPoints.begin(); it != controlPoints.end(); ++it) {
        if (it != controlPoints.begin())
            result << "," << std::endl << "          ";
        result << it->x << " " << it->y << " " << it->z;
    }

    result << " ]" << std::endl
           << "    }" << std::endl;
    result << "    NurbsSurface { " << std::endl
           << "      numUControlPoints " << numUControlPoints << std::endl
           << "      numVControlPoints " << numVControlPoints << std::endl
           << "      uKnotVector [ ";
    int index = 0;
    for (std::vector<float>::const_iterator it = uKnots.begin(); it != uKnots.end(); ++it) {
        result << *it;
        index++;
        if ((it+1) < uKnots.end()) {
            if (index % 4 == 0)
                result << "," << std::endl << "          ";
            else
                result << ", ";
        }
    }
    result << " ]" << std::endl
           << "      vKnotVector [ ";
    for (std::vector<float>::const_iterator it = vKnots.begin(); it != vKnots.end(); ++it) {
        result << *it;
        index++;
        if ((it+1) < vKnots.end()) {
            if (index % 4 == 0)
                result << "," << std::endl << "          ";
            else
                result << ", ";
        }
    }
    result << " ]" << std::endl
           << "    }" << std::endl
           << "  }" << std::endl;
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

void InventorBuilder::addBoundingBox(const Vector3f& pt1, const Vector3f& pt2, short lineWidth,
                                     float color_r,float color_g,float color_b)
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

    result << "  Separator { " << std::endl
           << "    Material { diffuseColor " << color_r << " "<< color_g << " "<< color_b << "} "  << std::endl
           << "    DrawStyle { lineWidth " << lineWidth << "} " << std::endl
           << "    Coordinate3 { " << std::endl
           << "      point [ "
           << "        " << pt[0].x << " " << pt[0].y << " " << pt[0].z << ",\n"
           << "        " << pt[1].x << " " << pt[1].y << " " << pt[1].z << ",\n"
           << "        " << pt[2].x << " " << pt[2].y << " " << pt[2].z << ",\n"
           << "        " << pt[3].x << " " << pt[3].y << " " << pt[3].z << ",\n"
           << "        " << pt[4].x << " " << pt[4].y << " " << pt[4].z << ",\n"
           << "        " << pt[5].x << " " << pt[5].y << " " << pt[5].z << ",\n"
           << "        " << pt[6].x << " " << pt[6].y << " " << pt[6].z << ",\n"
           << "        " << pt[7].x << " " << pt[7].y << " " << pt[7].z
           << "] " << std::endl
           << "    } " << std::endl
           << "    IndexedLineSet { coordIndex[ 0, 2, 6, 4, 0, -1\n"
              "        1, 5, 7, 3, 1, -1,\n"
              "        5, 4, 6, 7, 5, -1,\n"
              "        7, 6, 2, 3, 7, -1,\n"
              "        3, 2, 0, 1, 3, -1,\n"
              "        5, 1, 0, 4, 5, -1 ] } " << std::endl
           << "  } " << std::endl;
}

void InventorBuilder::addTransformation(const Matrix4D& transform)
{
    Vector3f cAxis, cBase;
    float fAngle = 0.0f, fTranslation = 0.0f;
    transform.toAxisAngle(cBase, cAxis,fAngle,fTranslation);
    cBase.x = static_cast<float>(transform[0][3]);
    cBase.y = static_cast<float>(transform[1][3]);
    cBase.z = static_cast<float>(transform[2][3]);
    addTransformation(cBase,cAxis,fAngle);
}

void InventorBuilder::addTransformation(const Vector3f& translation, const Vector3f& rotationaxis, float fAngle)
{
    result << Base::blanks(indent) << "Transform {" << std::endl;
    result << Base::blanks(indent) << "  translation "
         << translation.x << " " << translation.y << " " << translation.z << std::endl;
    result << Base::blanks(indent) << "  rotation "
         << rotationaxis.x << " " << rotationaxis.y << " " << rotationaxis.z
         << " " << fAngle << std::endl;
    result << Base::blanks(indent) <<  "}" << std::endl;
}
