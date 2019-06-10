/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>               *
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

#ifndef DRAWINGGUI_REZ_H
#define DRAWINGGUI_REZ_H

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <Base/Vector3D.h>
#include <Base/Tools2D.h>

namespace TechDrawGui
{

/// Functions to handle mm resolution conversion
class TechDrawGuiExport Rez
{
public:
    static double getParameter(void);
    static double getRezFactor(void);
    static void setRezFactor(double f);
//turn App side value to Gui side value
    static double guiX(double x);
    static Base::Vector3d guiX(Base::Vector3d v);
//turn Gui side value to App side value
    static double appX(double x);
    static Base::Vector3d appX(Base::Vector3d v);
    static QPointF appX(QPointF p);

    static QPointF guiPt(QPointF p);
    static QPointF appPt(QPointF p);

    static QRectF guiRect(QRectF r);
    static QSize guiSize(QSize s);
    static QSize appSize(QSize s);
    
private:
    static double m_rezFactor;
};

} //end namespace TechDrawGui
#endif
