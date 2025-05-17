/***************************************************************************
 *   Copyright (c) 2025 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2025 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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

#ifndef TECHDRAWGUI_USERTYPES_H
#define TECHDRAWGUI_USERTYPES_H

/*
Derived QGI Classes type() Values

Qt First UserType>> QGraphicsItem::UserType = 65536
*/

namespace TechDrawGui {
namespace UserType {

constexpr int QGIView            = QGraphicsItem::UserType + 101;
constexpr int QGIViewPart        = QGraphicsItem::UserType + 102;
constexpr int QGIEdge            = QGraphicsItem::UserType + 103;
constexpr int QGIFace            = QGraphicsItem::UserType + 104;
constexpr int QGIVertex          = QGraphicsItem::UserType + 105;
constexpr int QGIViewDimension   = QGraphicsItem::UserType + 106;
constexpr int QGIViewBalloon     = QGraphicsItem::UserType + 140;
constexpr int QGIBalloonLabel    = QGraphicsItem::UserType + 141;
constexpr int QGIDatumLabel      = QGraphicsItem::UserType + 107;
constexpr int QGIViewSection     = QGraphicsItem::UserType + 108;
constexpr int QGIArrow           = QGraphicsItem::UserType + 109;
constexpr int QGIViewCollection  = QGraphicsItem::UserType + 110;
constexpr int QGIProjGroup       = QGraphicsItem::UserType + 113;
constexpr int QGIViewAnnotation  = QGraphicsItem::UserType + 120;
constexpr int QGIViewSymbol      = QGraphicsItem::UserType + 121;
// constexpr int QGIHatch           = QGraphicsItem::UserType + 122;  //obsolete
constexpr int QGIClip            = QGraphicsItem::UserType + 123;
constexpr int QGISpreadsheet     = QGraphicsItem::UserType + 124;
constexpr int QGCustomText       = QGraphicsItem::UserType + 130;
constexpr int QGCustomSvg        = QGraphicsItem::UserType + 131;
constexpr int QGCustomClip       = QGraphicsItem::UserType + 132;
constexpr int QGCustomRect       = QGraphicsItem::UserType + 133;
constexpr int QGCustomLabel      = QGraphicsItem::UserType + 135;
constexpr int QGCustomBorder     = QGraphicsItem::UserType + 136;
constexpr int QGDisplayArea      = QGraphicsItem::UserType + 137;
constexpr int QGITemplate        = QGraphicsItem::UserType + 150;
constexpr int QGIDrawingTemplate = QGraphicsItem::UserType + 151;
constexpr int QGISVGTemplate     = QGraphicsItem::UserType + 153;
constexpr int TemplateTextField  = QGraphicsItem::UserType + 160;
constexpr int QGIPrimPath        = QGraphicsItem::UserType + 170;
constexpr int QGICMark           = QGraphicsItem::UserType + 171;
constexpr int QGISectionLine     = QGraphicsItem::UserType + 172;
constexpr int QGIDecoration      = QGraphicsItem::UserType + 173;
constexpr int QGICenterLine      = QGraphicsItem::UserType + 174;
constexpr int QGIDimLines        = QGraphicsItem::UserType + 175;
constexpr int QGIHighlight       = QGraphicsItem::UserType + 176;
constexpr int QGIGhostHighlight  = QGraphicsItem::UserType + 177;
constexpr int QGICaption         = QGraphicsItem::UserType + 180;
constexpr int QGIViewImage       = QGraphicsItem::UserType + 200;
constexpr int QGCustomImage      = QGraphicsItem::UserType + 201;
constexpr int QGIMatting         = QGraphicsItem::UserType + 205;
constexpr int QGTracker          = QGraphicsItem::UserType + 210;
constexpr int QGILeaderLine      = QGraphicsItem::UserType + 232;
constexpr int QGIRichAnno        = QGraphicsItem::UserType + 233;
constexpr int QGIBreakLine       = QGraphicsItem::UserType + 250;
constexpr int QGMText            = QGraphicsItem::UserType + 300;
constexpr int QGEPath            = QGraphicsItem::UserType + 301;
constexpr int QGMarker           = QGraphicsItem::UserType + 302;
constexpr int QGITile            = QGraphicsItem::UserType + 325;
constexpr int QGIWeldSymbol      = QGraphicsItem::UserType + 340;

}
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

#endif
