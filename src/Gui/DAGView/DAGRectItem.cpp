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
#ifndef _PreComp_
#include <QPainter>
#endif

#include "DAGRectItem.h"

using namespace Gui;
using namespace DAG;

RectItem::RectItem(QGraphicsItem* parent) : QGraphicsRectItem(parent)
{
  selected = false;
  preSelected = false;
  editing = false;
}

void RectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  //TODO figure out how to mimic painting of itemviews. QStyle, QStyledItemDelegate.
  
  QBrush brush = backgroundBrush;
  if (selected)
    brush = selectionBrush;
  if (preSelected)
    brush = preSelectionBrush;
  if (selected && preSelected)
    brush = bothBrush;
  if (editing)
    brush = editBrush;
  
  //heights are negative.
  float radius = std::min(this->rect().width(), std::fabs(this->rect().height())) * 0.1;
  painter->setBrush(brush);
  painter->setPen(this->pen()); //should be Qt::NoPen.
  painter->drawRoundedRect(this->rect(), radius, radius);
  
//   QGraphicsRectItem::paint(painter, option, widget);
}

#include <moc_DAGRectItem.cpp>
