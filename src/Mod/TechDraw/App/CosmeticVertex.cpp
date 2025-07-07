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

//! CosmeticVertex point is stored in unscaled, unrotated form

#include "PreCompiled.h"

#include <App/Application.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include "CosmeticVertex.h"
#include "CosmeticVertexPy.h"
#include "LineGroup.h"
#include "Preferences.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"

using namespace TechDraw;
using namespace std;
using DU = DrawUtil;

TYPESYSTEM_SOURCE(TechDraw::CosmeticVertex, Base::Persistence)

CosmeticVertex::CosmeticVertex() : TechDraw::Vertex()
{
    color = Preferences::vertexColor();
    size  = Preferences::vertexScale() *
            LineGroup::getDefaultWidth("Thin");

    hlrVisible = true;
    cosmetic = true;
    cosmeticTag = getTagAsString();
}

CosmeticVertex::CosmeticVertex(const TechDraw::CosmeticVertex* cv) : TechDraw::Vertex(cv),
    permaPoint(cv->permaPoint),
    linkGeom(cv->linkGeom),
    color(cv->color),
    size(cv->size),
    style(cv->style),
    visible(cv->visible)
{
    // Base fields
    hlrVisible = true;
    cosmetic = true;
    cosmeticTag = getTagAsString();
}

CosmeticVertex::CosmeticVertex(const Base::Vector3d& loc) : TechDraw::Vertex(loc),
    permaPoint(loc),
    color(Preferences::vertexColor())
{
    size  = Preferences::vertexScale() *
            LineGroup::getDefaultWidth("Thick");

    hlrVisible = true;
    cosmetic = true;
    cosmeticTag = getTagAsString();
}

void CosmeticVertex::move(const Base::Vector3d& newPos)
{
    point(newPos);
}

void CosmeticVertex::moveRelative(const Base::Vector3d& movement)
{
    point( point() += movement);
}

std::string CosmeticVertex::toString() const
{
    std::stringstream ss;
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
                 "\"/>" << '\n';
    writer.Stream() << writer.ind() << "<LinkGeom value=\"" <<  linkGeom << "\"/>" << '\n';
    writer.Stream() << writer.ind() << "<Color value=\"" <<  color.asHexString() << "\"/>" << '\n';
    writer.Stream() << writer.ind() << "<Size value=\"" <<  size << "\"/>" << '\n';
    writer.Stream() << writer.ind() << "<Style value=\"" <<  style << "\"/>" << '\n';
    const char v = visible?'1':'0';
    writer.Stream() << writer.ind() << "<Visible value=\"" <<  v << "\"/>" << '\n';

    //NOLINTNEXTLINE
    Tag::Save(writer);      // as "Tag"
}

void CosmeticVertex::Restore(Base::XMLReader &reader)
{
    if (!CosmeticVertex::restoreCosmetic()) {
        return;
    }
    TechDraw::Vertex::Restore(reader);

    // Vertex::Restore call to readNextElement may leave us already positioned on the PermaPoint element.
    if(strcmp(reader.localName(),"PermaPoint") != 0) {
        reader.readElement("PermaPoint");
    }
    permaPoint.x = reader.getAttribute<double>("X");
    permaPoint.y = reader.getAttribute<double>("Y");
    permaPoint.z = reader.getAttribute<double>("Z");
    reader.readElement("LinkGeom");
    linkGeom = reader.getAttribute<int>("value");
    reader.readElement("Color");
    std::string temp = reader.getAttribute<const char*>("value");
    color.fromHexString(temp);
    reader.readElement("Size");
    size = reader.getAttribute<double>("value");
    reader.readElement("Style");
    style = reader.getAttribute<int>("value");
    reader.readElement("Visible");
    visible = reader.getAttribute<bool>("value");

    Tag::Restore(reader);
}

Base::Vector3d CosmeticVertex::scaled(const double factor) const
{
    return permaPoint * factor;
}

//! returns a transformed version of our coordinates (permaPoint)
Base::Vector3d CosmeticVertex::rotatedAndScaled(const double scale, const double rotDegrees) const
{
    Base::Vector3d scaledPoint = scaled(scale);
    if (rotDegrees != 0.0) {
        // invert the Y coordinate so the rotation math works out
        // the stored point is inverted
        scaledPoint = DU::invertY(scaledPoint);
        scaledPoint.RotateZ(rotDegrees * std::numbers::pi / DegreesHalfCircle);
        scaledPoint = DU::invertY(scaledPoint);
    }
    return scaledPoint;
}

//! converts a point into its unscaled, unrotated form.  If point is Gui space coordinates,
//! it should be inverted (DU::invertY) before calling this method, and the result should be
//! inverted back on return.
Base::Vector3d CosmeticVertex::makeCanonicalPoint(DrawViewPart* dvp, Base::Vector3d point, bool unscale)
{
    double rotDeg = dvp->Rotation.getValue();

    Base::Vector3d result = point;
    if (rotDeg != 0.0) {
        // unrotate the point
        double rotRad = rotDeg * std::numbers::pi / DegreesHalfCircle;
        // we always rotate around the origin.
        result.RotateZ(-rotRad);
    }

    if (unscale) {
        // unrotated point is scaled and we need to unscale it (the normal situation)
        double scale = dvp->getScale();
        return result / scale;
    }

    // return the unrotated version of input point without unscaling
    return result;
}

//! a version of makeCanonicalPoint that accepts and returns an invertedPoint.
Base::Vector3d CosmeticVertex::makeCanonicalPointInverted(DrawViewPart* dvp, Base::Vector3d invertedPoint, bool unscale)
{
    Base::Vector3d result = makeCanonicalPoint(dvp,
                                               DU::invertY(invertedPoint),
                                               unscale);
    return DU::invertY(result);
}

CosmeticVertex* CosmeticVertex::copy() const
{
//    Base::Console().message("CV::copy()\n");
    return new CosmeticVertex(this);
}

CosmeticVertex* CosmeticVertex::clone() const
{
//    Base::Console().message("CV::clone()\n");
    CosmeticVertex* cpy = this->copy();
    cpy->setTag(this->getTag());
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
    Base::Console().message("CV::dump - %s \n", title);
    Base::Console().message("CV::dump - %s \n", toString().c_str());
}
