/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <cmath>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <App/Property.h>
#include <QDateTime>

#include "Constraint.h"
#include "ConstraintPy.h"


using namespace Sketcher;
using namespace Base;


TYPESYSTEM_SOURCE(Sketcher::Constraint, Base::Persistence)

const int Constraint::GeoUndef = -2000;

Constraint::Constraint()
: Value(0.0),
  Type(None),
  AlignmentType(Undef),
  Name(""),
  First(GeoUndef),
  FirstPos(none),
  Second(GeoUndef),
  SecondPos(none),
  Third(GeoUndef),
  ThirdPos(none),
  LabelDistance(10.f),
  LabelPosition(0.f),
  isDriving(true),
  InternalAlignmentIndex(-1),
  isInVirtualSpace(false)
{
    // Initialize a random number generator, to avoid Valgrind false positives.
    static boost::mt19937 ran;
    static bool seeded = false;

    if (!seeded) {
        ran.seed(QDateTime::currentMSecsSinceEpoch() & 0xffffffff);
        seeded = true;
    }
    static boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);

    tag = gen();
}

Constraint *Constraint::clone(void) const
{
    return new Constraint(*this);
}

Constraint *Constraint::copy(void) const
{
    Constraint *temp = new Constraint();
    temp->Value = this->Value;
    temp->Type = this->Type;
    temp->AlignmentType = this->AlignmentType;
    temp->Name = this->Name;
    temp->First = this->First;
    temp->FirstPos = this->FirstPos;
    temp->Second = this->Second;
    temp->SecondPos = this->SecondPos;
    temp->Third = this->Third;
    temp->ThirdPos = this->ThirdPos;
    temp->LabelDistance = this->LabelDistance;
    temp->LabelPosition = this->LabelPosition;
    temp->isDriving = this->isDriving;
    temp->InternalAlignmentIndex = this->InternalAlignmentIndex;
    temp->isInVirtualSpace = this->isInVirtualSpace;
    // Do not copy tag, otherwise it is considered a clone, and a "rename" by the expression engine.
    return temp;
}

PyObject *Constraint::getPyObject(void)
{
    return new ConstraintPy(new Constraint(*this));
}

Quantity Constraint::getPresentationValue() const
{
    Quantity quantity;
    switch (Type) {
    case Distance:
    case Radius:
    case Diameter:
    case DistanceX:
    case DistanceY:
        quantity.setValue(Value);
        quantity.setUnit(Unit::Length);
        break;
    case Angle:
        quantity.setValue(toDegrees<double>(Value));
        quantity.setUnit(Unit::Angle);
        break;
    case SnellsLaw:
        quantity.setValue(Value);
        break;
    default:
        quantity.setValue(Value);
        break;
    }

    QuantityFormat format = quantity.getFormat();
    format.option = QuantityFormat::None;
    format.format = QuantityFormat::Default;
    format.precision = 6; // QString's default
    quantity.setFormat(format);
    return quantity;
}

unsigned int Constraint::getMemSize (void) const
{
    return 0;
}

void Constraint::Save (Writer &writer) const
{
    std::string encodeName = encodeAttribute(Name);
    writer.Stream() << writer.ind()     << "<Constrain "
    << "Name=\""                        <<  encodeName              << "\" "
    << "Type=\""                        <<  (int)Type               << "\" ";
    if(this->Type==InternalAlignment)
        writer.Stream()
        << "InternalAlignmentType=\""   <<  (int)AlignmentType      << "\" "
        << "InternalAlignmentIndex=\""  <<  InternalAlignmentIndex  << "\" ";
    writer.Stream()
    << "Value=\""                       <<  Value                   << "\" "
    << "First=\""                       <<  First                   << "\" "
    << "FirstPos=\""                    <<  (int)  FirstPos         << "\" "
    << "Second=\""                      <<  Second                  << "\" "
    << "SecondPos=\""                   <<  (int) SecondPos         << "\" "
    << "Third=\""                       <<  Third                   << "\" "
    << "ThirdPos=\""                    <<  (int) ThirdPos          << "\" "
    << "LabelDistance=\""               <<  LabelDistance           << "\" "
    << "LabelPosition=\""               <<  LabelPosition           << "\" "
    << "IsDriving=\""                   <<  (int)isDriving          << "\" "
    << "IsInVirtualSpace=\""            <<  (int)isInVirtualSpace   << "\" />"

    << std::endl;
}

void Constraint::Restore(XMLReader &reader)
{
    reader.readElement("Constrain");
    Name      = reader.getAttribute("Name");
    Type      = (ConstraintType)  reader.getAttributeAsInteger("Type");
    Value     = reader.getAttributeAsFloat("Value");
    First     = reader.getAttributeAsInteger("First");
    FirstPos  = (PointPos)  reader.getAttributeAsInteger("FirstPos");
    Second    = reader.getAttributeAsInteger("Second");
    SecondPos = (PointPos)  reader.getAttributeAsInteger("SecondPos");

    if(this->Type==InternalAlignment) {
        AlignmentType = (InternalAlignmentType) reader.getAttributeAsInteger("InternalAlignmentType");

        if (reader.hasAttribute("InternalAlignmentIndex"))
            InternalAlignmentIndex = reader.getAttributeAsInteger("InternalAlignmentIndex");
    }
    else {
        AlignmentType = Undef;
    }

    // read the third geo group if present
    if (reader.hasAttribute("Third")) {
        Third    = reader.getAttributeAsInteger("Third");
        ThirdPos = (PointPos)  reader.getAttributeAsInteger("ThirdPos");
    }

    // Read the distance a constraint label has been moved
    if (reader.hasAttribute("LabelDistance"))
        LabelDistance = (float)reader.getAttributeAsFloat("LabelDistance");

    if (reader.hasAttribute("LabelPosition"))
        LabelPosition = (float)reader.getAttributeAsFloat("LabelPosition");

    if (reader.hasAttribute("IsDriving"))
        isDriving = reader.getAttributeAsInteger("IsDriving") ? true : false;

    if (reader.hasAttribute("IsInVirtualSpace"))
        isInVirtualSpace = reader.getAttributeAsInteger("IsInVirtualSpace") ? true : false;
}
