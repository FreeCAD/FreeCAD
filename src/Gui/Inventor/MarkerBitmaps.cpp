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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/nodes/SoMarkerSet.h>
# include <vector>
#endif

#include "MarkerBitmaps.h"

using namespace Gui::Inventor;

/*
from PySide import QtCore
from PySide import QtGui

def makeIcon(s):
  p=QtGui.QPixmap(s,s)
  painter=QtGui.QPainter(p)
  painter.setBrush(QtCore.Qt.SolidPattern)
  painter.drawEllipse(1,1,s-2,s-2)
  painter.end()

  buffer=QtCore.QBuffer()
  buffer.open(buffer.WriteOnly)
  p.save(buffer,"XPM")
  buffer.close()

  ary=buffer.buffer()
  lines=ary.split(",")
  ba=QtCore.QByteArray()
  for i in lines[3:]:
    ba = ba.append(i)

  ba=ba.replace("#","x")
  ba=ba.replace("."," ")
  print (ba.data())
*/

//CIRCLE_FILLED_11_11
const int CIRCLE11_WIDTH = 11;
const int CIRCLE11_HEIGHT = 11;
const char circle11_marker[CIRCLE11_WIDTH * CIRCLE11_HEIGHT + 1] = {
"           "
"   xxxxxx  "
"  xxxxxxxx "
"  xxxxxxxx "
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
"  xxxxxxxx "
"  xxxxxxxx "
"   xxxxxx  "};


//CIRCLE_FILLED_13_13
const int CIRCLE13_WIDTH = 13;
const int CIRCLE13_HEIGHT = 13;
const char circle13_marker[CIRCLE13_WIDTH * CIRCLE13_HEIGHT + 1] = {
"             "
"    xxxxxx   "
"   xxxxxxxx  "
"  xxxxxxxxxx "
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
"  xxxxxxxxxx "
"   xxxxxxxx  "
"    xxxxxx   "};


//CIRCLE_FILLED_15_15
const int CIRCLE15_WIDTH = 15;
const int CIRCLE15_HEIGHT = 15;
const char circle15_marker[CIRCLE15_WIDTH * CIRCLE15_HEIGHT + 1] = {
"               "
"     xxxxxx    "
"   xxxxxxxxxx  "
"  xxxxxxxxxxxx "
"  xxxxxxxxxxxx "
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
"  xxxxxxxxxxxx "
"   xxxxxxxxxxxx"
"   xxxxxxxxxx  "
"     xxxxxx    "};

std::map<MarkerBitmaps::Marker, int> MarkerBitmaps::markerIndex;

void
MarkerBitmaps::initClass()
{
    createBitmap("CIRCLE_FILLED", 11, 11, 11, circle11_marker);
    createBitmap("CIRCLE_FILLED", 13, 13, 13, circle13_marker);
    createBitmap("CIRCLE_FILLED", 15, 15, 15, circle15_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("CIRCLE_FILLED", 9)] = SoMarkerSet::CIRCLE_FILLED_9_9;
    markerIndex [std::make_pair("CIRCLE_FILLED", 7)] = SoMarkerSet::CIRCLE_FILLED_7_7;
    markerIndex [std::make_pair("CIRCLE_FILLED", 5)] = SoMarkerSet::CIRCLE_FILLED_5_5;
}

void MarkerBitmaps::createBitmap(const std::string& name, int px, int width, int height, const char* marker)
{
    int byteidx = 0;
    const int byteWidth = (width + 7) / 2;
    int size = byteWidth * height;
    std::vector<unsigned char> bitmapbytes(size);

    for (int h = 0; h < height; h++) {
        unsigned char bits = 0;
        for (int w = 0; w < width; w++) {
            if (marker[(h * width) + w] != ' ') {
                bits |= (0x80 >> (w % 8));
            }
            if ((((w + 1) % 8) == 0) || (w == width - 1)) {
                bitmapbytes[byteidx++] = bits;
                bits = 0;
            }
        }
    }

    int MY_BITMAP_IDX = SoMarkerSet::getNumDefinedMarkers(); // add at end
    SoMarkerSet::addMarker(MY_BITMAP_IDX, SbVec2s(width, height),
                           &(bitmapbytes[0]), FALSE, TRUE);

    markerIndex[std::make_pair(name, px)] = MY_BITMAP_IDX;
}

int MarkerBitmaps::getMarkerIndex(const std::string& name, int px)
{
    std::map<Marker, int>::iterator it = markerIndex.find(std::make_pair(name, px));
    if (it != markerIndex.end()) {
        return it->second;
    }

    return static_cast<int>(SoMarkerSet::CIRCLE_FILLED_7_7);
}

std::list<int> MarkerBitmaps::getSupportedSizes(const std::string& name)
{
    std::list<int> sizes;
    for (std::map<Marker, int>::iterator it = markerIndex.begin(); it != markerIndex.end(); ++it) {
        if (it->first.first == name)
            sizes.push_back(it->first.second);
    }
    return sizes;
}
