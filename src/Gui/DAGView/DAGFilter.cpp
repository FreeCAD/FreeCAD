/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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

#include <App/DocumentObject.h>
#include <Base/Type.h>

#include "DAGFilter.h"


using namespace Gui;
using namespace DAG;

FilterBase::FilterBase() : name(QStringLiteral("empty name"))
{

}

FilterTyped::FilterTyped(const std::string &typeIn) : FilterBase(), type(typeIn)
{
  name = QString::fromStdString(typeIn);
}

bool FilterTyped::goFilter(const Gui::DAG::Vertex& vertexIn, const Graph& graphIn, const GraphLinkContainer& linkIn) const
{
  Q_UNUSED(graphIn);
  if (type.empty())
    return false;
  Base::Type theType = Base::Type::fromName(type.c_str());
  if (theType.isBad())
    return false;

  const GraphLinkRecord &sourceRecord = findRecord(vertexIn, linkIn);
  if (sourceRecord.DObject->getTypeId() == theType)
    return true;

  return false;
}
