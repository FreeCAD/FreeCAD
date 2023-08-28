/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                <jrheinlaender[at]users.sourceforge.net> *
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

#include "DatumFeature.h"
#include "OCCError.h"
#include "PartPyCXX.h"


using namespace Part;
using namespace Attacher;

PROPERTY_SOURCE_ABSTRACT_WITH_EXTENSIONS(Part::Datum, Part::Feature)

Datum::Datum()
{
    AttachExtension::initExtension(this);
    touch();
}

Datum::~Datum() = default;

void Datum::onDocumentRestored()
{
    // This seems to be the only way to make the ViewProvider display the datum feature
    Support.touch();
    Part::Feature::onDocumentRestored();
}

TopoDS_Shape Datum::getShape() const
{
    Part::TopoShape sh = Shape.getShape();
    sh.setPlacement(Placement.getValue());
    return sh.getShape();
}

App::DocumentObject *Datum::getSubObject(const char *subname,
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const
{
    // For the sake of simplicity, we don't bother to check for subname, just
    // return the shape as it is, because a datum object only holds shape with
    // one single geometry element.
    (void)subname;
    (void)depth;

    if(pmat && transform)
        *pmat *= Placement.getValue().toMatrix();

    if(!pyObj)
        return const_cast<Datum*>(this);

    Base::PyGILStateLocker lock;
    PY_TRY {
        TopoShape ts(getShape().Located(TopLoc_Location()));
        if(pmat && !ts.isNull())
            ts.transformShape(*pmat,false,true);
        *pyObj =  Py::new_reference_to(shape2pyshape(ts.getShape()));
        return const_cast<Datum*>(this);
    } PY_CATCH_OCC
}

Base::Vector3d Datum::getBasePoint () const {
    return Placement.getValue().getPosition();
}

void Datum::handleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName)
{
    extHandleChangedPropertyName(reader, TypeName, PropName); // AttachExtension
}
