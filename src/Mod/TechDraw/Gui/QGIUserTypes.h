/***************************************************************************
 *   Copyright (c) 2025 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2025 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software, you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation, either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY, without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library, see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QGraphicsItem>

/*
Derived QGI Classes type() Values

Qt First UserType>> QGraphicsItem::UserType = 65536
*/

namespace TechDrawGui {
struct UserType {
enum : int {
    QGCustomBorder = QGraphicsItem::UserType,
    QGCustomClip,
    QGCustomImage,
    QGCustomLabel,
    QGCustomRect,
    QGCustomSvg,
    QGCustomText,
    QGDisplayArea,
    QGEPath,
    QGIArrow,
    QGIBalloonLabel,
    QGIBreakLine,
    QGICaption,
    QGICenterLine,
    QGICMark,
    QGIDatumLabel,
    QGIDecoration,
    QGIDimLines,
    QGIDrawingTemplate,
    QGIEdge,
    QGIFace,
    QGIGhostHighlight,
    //QGIHatch,  //obsolete
    QGIHighlight,
    QGILeaderLine,
    QGIMatting,
    QGIPrimPath,
    QGIProjGroup,
    QGIRichAnno,
    QGISectionLine,
    QGISpreadsheet,
    QGISVGTemplate,
    QGITemplate,
    QGITile,
    QGIVertex,
    QGIView,
    QGIViewAnnotation,
    QGIViewBalloon,
    QGIViewClip,
    QGIViewCollection,
    QGIViewDimension,
    QGIViewImage,
    QGIViewPart,
    QGIViewSection,
    QGIViewSymbol,
    QGIWeldSymbol,
    QGMarker,
    QGMText,
    QGTracker,
    TemplateTextField
};
};
}

/*
Standard Types
path 2
rect 3
ellipse 4
polygon 5
line 6
pixmap 7
text 8
simpletext 9
group 10
*/