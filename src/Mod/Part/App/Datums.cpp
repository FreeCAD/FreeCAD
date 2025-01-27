/***************************************************************************
 *   Copyright (c) 2024 Ondsel (PL Boyer) <development@ondsel.com>         *
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

#include "Datums.h"


using namespace Part;
using namespace Attacher;

PROPERTY_SOURCE_WITH_EXTENSIONS(Part::DatumPlane, App::Plane)

Part::DatumPlane::DatumPlane()
{
    AttachExtension::initExtension(this);
    this->setAttacher(new AttachEnginePlane);
}


PROPERTY_SOURCE_WITH_EXTENSIONS(Part::DatumLine, App::Line)

Part::DatumLine::DatumLine()
{
    setBaseDirection(Base::Vector3d(0, 0, 1));
    AttachExtension::initExtension(this);
    this->setAttacher(new AttachEngineLine);
}


PROPERTY_SOURCE_WITH_EXTENSIONS(Part::DatumPoint, App::Point)

Part::DatumPoint::DatumPoint()
{
    AttachExtension::initExtension(this);
    this->setAttacher(new AttachEnginePoint);
}


PROPERTY_SOURCE_WITH_EXTENSIONS(Part::LocalCoordinateSystem, App::LocalCoordinateSystem)

Part::LocalCoordinateSystem::LocalCoordinateSystem()
{
    AttachExtension::initExtension(this);
}
