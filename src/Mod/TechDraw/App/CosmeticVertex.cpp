/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2022 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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
    #include <boost/uuid/uuid_generators.hpp>
    #include <boost/uuid/uuid_io.hpp>
#endif // _PreComp_

#include <App/Application.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include "CosmeticVertex.h"
#include "CosmeticVertexPy.h"
#include "LineGroup.h"
#include "Preferences.h"
#include "DrawUtil.h"

using namespace TechDraw;
using namespace std;
using DU = DrawUtil;

TYPESYSTEM_SOURCE(TechDraw::CosmeticVertex, Base::Persistence)

CosmeticVertex::CosmeticVertex() : TechDraw::Vertex()
{
    point(Base::Vector3d(0.0, 0.0, 0.0));
    permaPoint = Base::Vector3d(0.0, 0.0, 0.0);
    linkGeom = -1;
    color = Preferences::vertexColor();
    size  = Preferences::vertexScale() *
            LineGroup::getDefaultWidth("Thin");
    style = 1;
    visible = true;
    hlrVisible = true;
    cosmetic = true;

    createNewTag();
}

CosmeticVertex::CosmeticVertex(const TechDraw::CosmeticVertex* cv) : TechDraw::Vertex(cv)
{
    permaPoint = cv->permaPoint;
    linkGeom = cv->linkGeom;
    color = cv->color;
    size  = cv->size;
    style = cv->style;
    visible = cv->visible;
    hlrVisible = true;
    cosmetic = true;

    createNewTag();
}

CosmeticVertex::CosmeticVertex(const Base::Vector3d& loc) : TechDraw::Vertex(loc)
{
//    Base::Console().Message("CV::CV(%s)\n", DU::formatVector(loc).c_str());
    permaPoint = loc;
    linkGeom = -1;
    color = Preferences::vertexColor();
    size  = Preferences::vertexScale() *
            LineGroup::getDefaultWidth("Thick");
    style = 1;        //TODO: implement styled vertexes
    visible = true;
    hlrVisible = true;
    cosmetic = true;

    createNewTag();

}

void CosmeticVertex::move(const Base::Vector3d& newPos)
{
    permaPoint = newPos;
}

void CosmeticVertex::moveRelative(const Base::Vector3d& movement)
{
    permaPoint += movement;
}

std::string CosmeticVertex::toString() const
{
    std::stringstream ss;
    ss << permaPoint.x << ", " <<
          permaPoint.y << ", " <<
          permaPoint.z << ", " <<
          " / ";
    ss << point().x << ", " <<
          point().y << ", " <<
          point().z << ", " <<
          " / " <<
          linkGeom << ", " <<
          " / " <<
          color.asHexString() << ", "  <<
          " / " <<
          size << ", " <<
          " / " <<
          style << ", "  <<
          " / " <<
          visible << " / " ;
    ss << getTagAsString();
    return ss.str();
}

// Persistence implementers
unsigned int CosmeticVertex::getMemSize () const
{
    return 1;
}

void CosmeticVertex::Save(Base::Writer &writer) const
{
    TechDraw::Vertex::Save(writer);
    writer.Stream() << writer.ind() << "<PermaPoint "
                << "X=\"" <<  permaPoint.x <<
                "\" Y=\"" <<  permaPoint.y <<
                "\" Z=\"" <<  permaPoint.z <<
                 "\"/>" << endl;
    writer.Stream() << writer.ind() << "<LinkGeom value=\"" <<  linkGeom << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Color value=\"" <<  color.asHexString() << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Size value=\"" <<  size << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Style value=\"" <<  style << "\"/>" << endl;
    const char v = visible?'1':'0';
    writer.Stream() << writer.ind() << "<Visible value=\"" <<  v << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Tag value=\"" <<  getTagAsString() << "\"/>" << endl;
}

void CosmeticVertex::Restore(Base::XMLReader &reader)
{
    if (!CosmeticVertex::restoreCosmetic()) {
        return;
    }
    TechDraw::Vertex::Restore(reader);
    reader.readElement("PermaPoint");
    permaPoint.x = reader.getAttributeAsFloat("X");
    permaPoint.y = reader.getAttributeAsFloat("Y");
    permaPoint.z = reader.getAttributeAsFloat("Z");
    reader.readElement("LinkGeom");
    linkGeom = reader.getAttributeAsInteger("value");
    reader.readElement("Color");
    std::string temp = reader.getAttribute("value");
    color.fromHexString(temp);
    reader.readElement("Size");
    size = reader.getAttributeAsFloat("value");
    reader.readElement("Style");
    style = reader.getAttributeAsInteger("value");
    reader.readElement("Visible");
    visible = (int)reader.getAttributeAsInteger("value")==0?false:true;
    reader.readElement("Tag");
    temp = reader.getAttribute("value");
    boost::uuids::string_generator gen;
    boost::uuids::uuid u1 = gen(temp);
    tag = u1;
}

Base::Vector3d CosmeticVertex::scaled(const double factor)
{
    return permaPoint * factor;
}

Base::Vector3d CosmeticVertex::rotatedAndScaled(const double scale, const double rotDegrees)
{
    Base::Vector3d scaledPoint = scaled(scale);
    if (rotDegrees != 0.0) {
        // invert the Y coordinate so the rotation math works out
        scaledPoint = DU::invertY(scaledPoint);
        scaledPoint.RotateZ(rotDegrees * M_PI / 180.0);
        scaledPoint = DU::invertY(scaledPoint);
    }
    return scaledPoint;
}

boost::uuids::uuid CosmeticVertex::getTag() const
{
    return tag;
}

std::string CosmeticVertex::getTagAsString() const
{
    return boost::uuids::to_string(getTag());
}

void CosmeticVertex::createNewTag()
{
    // Initialize a random number generator, to avoid Valgrind false positives.
    static boost::mt19937 ran;
    static bool seeded = false;

    if (!seeded) {
        ran.seed(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    static boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);

    tag = gen();
}

void CosmeticVertex::assignTag(const TechDraw::CosmeticVertex* cv)
{
    if(cv->getTypeId() == this->getTypeId())
        this->tag = cv->tag;
    else
        throw Base::TypeError("CosmeticVertex tag can not be assigned as types do not match.");
}

CosmeticVertex* CosmeticVertex::copy() const
{
//    Base::Console().Message("CV::copy()\n");
    return new CosmeticVertex(this);
}

CosmeticVertex* CosmeticVertex::clone() const
{
//    Base::Console().Message("CV::clone()\n");
    CosmeticVertex* cpy = this->copy();
    cpy->tag = this->tag;
    return cpy;
}

PyObject* CosmeticVertex::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new CosmeticVertexPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// To do: make const
void CosmeticVertex::dump(const char* title)
{
    Base::Console().Message("CV::dump - %s \n", title);
    Base::Console().Message("CV::dump - %s \n", toString().c_str());
}
