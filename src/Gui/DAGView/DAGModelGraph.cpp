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

#include "DAGModelGraph.h"


using namespace Gui;
using namespace DAG;

VertexProperty::VertexProperty() :
  rectangle(new RectItem()),
  point(new QGraphicsEllipseItem()),
  visibleIcon(new QGraphicsPixmapItem()),
  stateIcon(new QGraphicsPixmapItem()),
  icon(new QGraphicsPixmapItem()),
  text(new QGraphicsTextItem())
{
  //set z values.
  this->rectangle->setZValue(-1000.0);
  this->point->setZValue(1000.0);
  this->visibleIcon->setZValue(0.0);
  this->stateIcon->setZValue(0.0);
  this->icon->setZValue(0.0);
  this->text->setZValue(0.0);
}

EdgeProperty::EdgeProperty() = default;

bool Gui::DAG::hasRecord(const App::DocumentObject* dObjectIn, const GraphLinkContainer &containerIn)
{
  using List = GraphLinkContainer::index<GraphLinkRecord::ByDObject>::type;
  const List &list = containerIn.get<GraphLinkRecord::ByDObject>();
  List::const_iterator it = list.find(dObjectIn);
  return it != list.end();
}

const GraphLinkRecord& Gui::DAG::findRecord(Vertex vertexIn, const GraphLinkContainer &containerIn)
{
  using List = GraphLinkContainer::index<GraphLinkRecord::ByVertex>::type;
  const List &list = containerIn.get<GraphLinkRecord::ByVertex>();
  List::const_iterator it = list.find(vertexIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Gui::DAG::findRecord(const App::DocumentObject* dObjectIn, const GraphLinkContainer &containerIn)
{
  using List = GraphLinkContainer::index<GraphLinkRecord::ByDObject>::type;
  const List &list = containerIn.get<GraphLinkRecord::ByDObject>();
  List::const_iterator it = list.find(dObjectIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Gui::DAG::findRecord(const ViewProviderDocumentObject* VPDObjectIn, const GraphLinkContainer &containerIn)
{
  using List = GraphLinkContainer::index<GraphLinkRecord::ByVPDObject>::type;
  const List &list = containerIn.get<GraphLinkRecord::ByVPDObject>();
  List::const_iterator it = list.find(VPDObjectIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Gui::DAG::findRecord(const RectItem* rectIn, const GraphLinkContainer &containerIn)
{
  using List = GraphLinkContainer::index<GraphLinkRecord::ByRectItem>::type;
  const List &list = containerIn.get<GraphLinkRecord::ByRectItem>();
  List::const_iterator it = list.find(rectIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Gui::DAG::findRecord(const std::string &stringIn, const GraphLinkContainer &containerIn)
{
  using List = GraphLinkContainer::index<GraphLinkRecord::ByUniqueName>::type;
  const List &list = containerIn.get<GraphLinkRecord::ByUniqueName>();
  List::const_iterator it = list.find(stringIn);
  assert(it != list.end());
  return *it;
}

void Gui::DAG::eraseRecord(const ViewProviderDocumentObject* VPDObjectIn, GraphLinkContainer &containerIn)
{
  using List = GraphLinkContainer::index<GraphLinkRecord::ByVPDObject>::type;
  const List &list = containerIn.get<GraphLinkRecord::ByVPDObject>();
  List::iterator it = list.find(VPDObjectIn);
  assert(it != list.end());
  containerIn.get<GraphLinkRecord::ByVPDObject>().erase(it);
}
