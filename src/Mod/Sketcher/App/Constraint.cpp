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
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>


#include "Constraint.h"
#include "ConstraintPy.h"


using namespace Sketcher;
using namespace Base;


TYPESYSTEM_SOURCE(Sketcher::Constraint, Base::Persistence)

const int Constraint::GeoUndef = -2000;

Constraint::Constraint()
: Type(None),
  Name(""),
  Value(0.0),
  First(GeoUndef),
  FirstPos(none),
  Second(GeoUndef),
  SecondPos(none),
  Third(GeoUndef),
  ThirdPos(none),
  LabelDistance(10.f),
  LabelPosition(0.f)
{
}

Constraint::Constraint(const Constraint& from)
: Type(from.Type),
  Name(from.Name),
  Value(from.Value),
  First(from.First),
  FirstPos(from.FirstPos),
  Second(from.Second),
  SecondPos(from.SecondPos),
  Third(from.Third),
  ThirdPos(from.ThirdPos),
  LabelDistance(from.LabelDistance),
  LabelPosition(from.LabelPosition)
{
}

Constraint::~Constraint()
{
}

Constraint *Constraint::clone(void) const
{
    return new Constraint(*this);
}

PyObject *Constraint::getPyObject(void)
{
    return new ConstraintPy(new Constraint(*this));
}

unsigned int Constraint::getMemSize (void) const
{
    return 0;
}

void Constraint::Save (Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Constrain "
    << "Name=\""          <<  Name            << "\" "
    << "Type=\""          <<  (int)Type       << "\" "
    << "Value=\""         <<  Value           << "\" "
    << "First=\""         <<  First           << "\" "
    << "FirstPos=\""      <<  (int)  FirstPos << "\" "
    << "Second=\""        <<  Second          << "\" "
    << "SecondPos=\""     <<  (int) SecondPos << "\" "
    << "Third=\""         <<  Third           << "\" "
    << "ThirdPos=\""      <<  (int) ThirdPos  << "\" "
    << "LabelDistance=\"" <<  LabelDistance   << "\" "
    << "LabelPosition=\"" <<  LabelPosition   << "\" />"
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
}
