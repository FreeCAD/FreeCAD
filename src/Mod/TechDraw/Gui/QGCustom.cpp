/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
#include <QGraphicsItemGroup>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QGraphicsTextItem>
#include <QPointF>
#include <QRectF>
#endif

#include <Base/Console.h>
#include <Mod/TechDraw/TechDrawGlobal.h>
#include "QGCustom.h"

using namespace TechDrawGui;

/**
 * @brief Centers the object at a given position
 * 
 * @param centerPos is the position to center at
 */
template<typename T>
void QGCustom<T>::centerAt(QPointF position)
{
    QRectF box = this->boundingRect();
    box.moveCenter(position);
    this->setPos(box.topLeft());
}

/**
 * @brief Centers the object at a given position
 * 
 * @param x is the x coordinate to center at
 * @param y is the y coordinate to center at
 */
template<typename T>
void QGCustom<T>::centerAt(double x, double y)
{
    centerAt(QPointF(x, y));
}

template class QGCustom<QGraphicsItemGroup>;
template class QGCustom<QGraphicsPixmapItem>;
template class QGCustom<QGraphicsRectItem>;
template class QGCustom<QGraphicsSvgItem>;
template class QGCustom<QGraphicsTextItem>;
