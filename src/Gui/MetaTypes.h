/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_METATYPES_H
#define GUI_METATYPES_H

#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/Quantity.h>

Q_DECLARE_METATYPE(Base::Vector3f)
Q_DECLARE_METATYPE(Base::Vector3d)
Q_DECLARE_METATYPE(Base::Matrix4D)
Q_DECLARE_METATYPE(Base::Placement)
Q_DECLARE_METATYPE(Base::Quantity)
Q_DECLARE_METATYPE(QList<Base::Quantity>)

#endif // GUI_METATYPES_H
