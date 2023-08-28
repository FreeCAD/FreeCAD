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

//DIAMOND_FILLED_11_11
const int DIAMOND_FILLED11_WIDTH = 11;
const int DIAMOND_FILLED11_HEIGHT = 11;
const char diamondFilled11_marker[DIAMOND_FILLED11_WIDTH * DIAMOND_FILLED11_HEIGHT + 1] = {
"           "
"     xx    "
"    xxxx   "
"   xxxxxx  "
"  xxxxxxxx "
" xxxxxxxxxx"
" xxxxxxxxxx"
"  xxxxxxxx "
"   xxxxxx  "
"    xxxx   "
"     xx    "};


//DIAMOND_FILLED_13_13
const int DIAMOND_FILLED13_WIDTH = 13;
const int DIAMOND_FILLED13_HEIGHT = 13;
const char diamondFilled13_marker[DIAMOND_FILLED13_WIDTH * DIAMOND_FILLED13_HEIGHT + 1] = {
"             "
"      xx     "
"     xxxx    "
"    xxxxxx   "
"   xxxxxxxx  "
"  xxxxxxxxxx "
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
"  xxxxxxxxxx "
"   xxxxxxxx  "
"    xxxxxx   "
"     xxxx    "
"      xx     "};


//DIAMOND_FILLED_15_15
const int DIAMOND_FILLED15_WIDTH = 15;
const int DIAMOND_FILLED15_HEIGHT = 15;
const char diamondFilled15_marker[DIAMOND_FILLED15_WIDTH * DIAMOND_FILLED15_HEIGHT + 1] = {
"               "
"       xx      "
"      xxxx     "
"     xxxxxx    "
"    xxxxxxxx   "
"   xxxxxxxxxx  "
"  xxxxxxxxxxxx "
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
"  xxxxxxxxxxxx "
"   xxxxxxxxxx  "
"    xxxxxxxx   "
"     xxxxxx    "
"      xxxx     "
"       xx      "};

//CROSS_11_11
const int CROSS11_WIDTH = 11;
const int CROSS11_HEIGHT = 11;
const char cross11_marker[CROSS11_WIDTH * CROSS11_HEIGHT + 1] = {
"           "
" xx      xx"
"  xx    xx "
"   xx  xx  "
"    xxxx   "
"     xx    "
"     xx    "
"    xxxx   "
"   xx  xx  "
"  xx    xx "
" xx      xx"};


//CROSS_13_13
const int CROSS13_WIDTH = 13;
const int CROSS13_HEIGHT = 13;
const char cross13_marker[CROSS13_WIDTH * CROSS13_HEIGHT + 1] = {
"             "
" xx        xx"
"  xx      xx "
"   xx    xx  "
"    xx  xx   "
"     xxxx    "
"      xx     "
"      xx     "
"     xxxx    "
"    xx  xx   "
"   xx    xx  "
"  xx      xx "
" xx        xx"};


//CROSS_15_15
const int CROSS15_WIDTH = 15;
const int CROSS15_HEIGHT = 15;
const char cross15_marker[CROSS15_WIDTH * CROSS15_HEIGHT + 1] = {
"               "
" xx          xx"
"  xx        xx "
"   xx      xx  "
"    xx    xx   "
"     xx  xx    "
"      xxxx     "
"       xx      "
"       xx      "
"      xxxx     "
"     xx  xx    "
"    xx    xx   "
"   xx      xx  "
"  xx        xx "
" xx          xx"};

//PLUS_11_11
const int PLUS11_WIDTH = 11;
const int PLUS11_HEIGHT = 11;
const char plus11_marker[PLUS11_WIDTH * PLUS11_HEIGHT + 1] = {
    "           "
    "     xx    "
    "     xx    "
    "     xx    "
    "     xx    "
    " xxxxxxxxxx"
    " xxxxxxxxxx"
    "     xx    "
    "     xx    "
    "     xx    "
    "     xx    "};


//PLUS_13_13
const int PLUS13_WIDTH = 13;
const int PLUS13_HEIGHT = 13;
const char plus13_marker[PLUS13_WIDTH * PLUS13_HEIGHT + 1] = {
    "             "
    "      xx     "
    "      xx     "
    "      xx     "
    "      xx     "
    "      xx     "
    " xxxxxxxxxxxx"
    " xxxxxxxxxxxx"
    "      xx     "
    "      xx     "
    "      xx     "
    "      xx     "
    "      xx     "};


//PLUS_15_15
const int PLUS15_WIDTH = 15;
const int PLUS15_HEIGHT = 15;
const char plus15_marker[PLUS15_WIDTH * PLUS15_HEIGHT + 1] = {
    "               "
    "       xx      "
    "       xx      "
    "       xx      "
    "       xx      "
    "       xx      "
    "       xx      "
    " xxxxxxxxxxxxxx"
    " xxxxxxxxxxxxxx"
    "       xx      "
    "       xx      "
    "       xx      "
    "       xx      "
    "       xx      "
    "       xx      "};

//SQUARE_LINE_11_11
const int SQUARE_LINE11_WIDTH = 11;
const int SQUARE_LINE11_HEIGHT = 11;
const char squareLine11_marker[SQUARE_LINE11_WIDTH * SQUARE_LINE11_HEIGHT + 1] = {
"           "
" xxxxxxxxxx"
" xxxxxxxxxx"
" xx      xx"
" xx      xx"
" xx      xx"
" xx      xx"
" xx      xx"
" xx      xx"
" xxxxxxxxxx"
" xxxxxxxxxx"};


//SQUARE_LINE_13_13
const int SQUARE_LINE13_WIDTH = 13;
const int SQUARE_LINE13_HEIGHT = 13;
const char squareLine13_marker[SQUARE_LINE13_WIDTH * SQUARE_LINE13_HEIGHT + 1] = {
"             "
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"};


//SQUARE_LINE_15_15
const int SQUARE_LINE15_WIDTH = 15;
const int SQUARE_LINE15_HEIGHT = 15;
const char squareLine15_marker[SQUARE_LINE15_WIDTH * SQUARE_LINE15_HEIGHT + 1] = {
"               "
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"};

//SQUARE_FILLED_11_11
const int SQUARE_FILLED11_WIDTH = 11;
const int SQUARE_FILLED11_HEIGHT = 11;
const char squareFilled11_marker[SQUARE_FILLED11_WIDTH * SQUARE_FILLED11_HEIGHT + 1] = {
"           "
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"};


//SQUARE_FILLED_13_13
const int SQUARE_FILLED13_WIDTH = 13;
const int SQUARE_FILLED13_HEIGHT = 13;
const char squareFilled13_marker[SQUARE_FILLED13_WIDTH * SQUARE_FILLED13_HEIGHT + 1] = {
"             "
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"
" xxxxxxxxxxxx"};


//SQUARE_FILLED_15_15
const int SQUARE_FILLED15_WIDTH = 15;
const int SQUARE_FILLED15_HEIGHT = 15;
const char squareFilled15_marker[SQUARE_FILLED15_WIDTH * SQUARE_FILLED15_HEIGHT + 1] = {
"               "
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"
" xxxxxxxxxxxxxx"};

//CIRCLE_LINE_11_11
const int CIRCLE_LINE11_WIDTH = 11;
const int CIRCLE_LINE11_HEIGHT = 11;
const char circleLine11_marker[CIRCLE_LINE11_WIDTH * CIRCLE_LINE11_HEIGHT + 1] = {
"           "
"   xxxxxx  "
"  xxxxxxxx "
" xxx    xxx"
" xx      xx"
" xx      xx"
" xx      xx"
" xx      xx"
" xxx    xxx"
"  xxxxxxxx "
"   xxxxxx  "};


//CIRCLE_LINE_13_13
const int CIRCLE_LINE13_WIDTH = 13;
const int CIRCLE_LINE13_HEIGHT = 13;
const char circleLine13_marker[CIRCLE_LINE13_WIDTH * CIRCLE_LINE13_HEIGHT + 1] = {
"             "
"    xxxxxx   "
"   xxxxxxxx  "
"  xx      xx "
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
" xx        xx"
"  xx      xx "
"   xxxxxxxx  "
"    xxxxxx   "};


//CIRCLE_LINE_15_15
const int CIRCLE_LINE15_WIDTH = 15;
const int CIRCLE_LINE15_HEIGHT = 15;
const char circleLine15_marker[CIRCLE_LINE15_WIDTH * CIRCLE_LINE15_HEIGHT + 1] = {
"               "
"     xxxxxx    "
"   xxxxxxxxxx  "
"  xxx      xxx "
"  xx        xx "
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
" xx          xx"
"  xx        xx "
"  xxx      xxx "
"   xxxxxxxxxx  "
"     xxxxxx    "};

//CIRCLE_FILLED_11_11
const int CIRCLE_FILLED11_WIDTH = 11;
const int CIRCLE_FILLED11_HEIGHT = 11;
const char circleFilled11_marker[CIRCLE_FILLED11_WIDTH * CIRCLE_FILLED11_HEIGHT + 1] = {
"           "
"   xxxxxx  "
"  xxxxxxxx "
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
" xxxxxxxxxx"
"  xxxxxxxx "
"   xxxxxx  "};


//CIRCLE_FILLED_13_13
const int CIRCLE_FILLED13_WIDTH = 13;
const int CIRCLE_FILLED13_HEIGHT = 13;
const char circleFilled13_marker[CIRCLE_FILLED13_WIDTH * CIRCLE_FILLED13_HEIGHT + 1] = {
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
const int CIRCLE_FILLED15_WIDTH = 15;
const int CIRCLE_FILLED15_HEIGHT = 15;
const char circleFilled15_marker[CIRCLE_FILLED15_WIDTH * CIRCLE_FILLED15_HEIGHT + 1] = {
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
"  xxxxxxxxxxxx "
"   xxxxxxxxxx  "
"     xxxxxx    "};

std::map<MarkerBitmaps::Marker, int> MarkerBitmaps::markerIndex;

void
MarkerBitmaps::initClass()
{
    createBitmap("DIAMOND_FILLED", 11, 11, 11, diamondFilled11_marker);
    createBitmap("DIAMOND_FILLED", 13, 13, 13, diamondFilled13_marker);
    createBitmap("DIAMOND_FILLED", 15, 15, 15, diamondFilled15_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("DIAMOND_FILLED", 9)] = SoMarkerSet::DIAMOND_FILLED_9_9;
    markerIndex [std::make_pair("DIAMOND_FILLED", 7)] = SoMarkerSet::DIAMOND_FILLED_7_7;
    markerIndex [std::make_pair("DIAMOND_FILLED", 5)] = SoMarkerSet::DIAMOND_FILLED_5_5;

    createBitmap("CROSS", 11, 11, 11, cross11_marker);
    createBitmap("CROSS", 13, 13, 13, cross13_marker);
    createBitmap("CROSS", 15, 15, 15, cross15_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("CROSS", 9)] = SoMarkerSet::CROSS_9_9;
    markerIndex [std::make_pair("CROSS", 7)] = SoMarkerSet::CROSS_7_7;
    markerIndex [std::make_pair("CROSS", 5)] = SoMarkerSet::CROSS_5_5;

    createBitmap("PLUS", 11, 11, 11, plus11_marker);
    createBitmap("PLUS", 13, 13, 13, plus13_marker);
    createBitmap("PLUS", 15, 15, 15, plus15_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("PLUS", 9)] = SoMarkerSet::PLUS_9_9;
    markerIndex [std::make_pair("PLUS", 7)] = SoMarkerSet::PLUS_7_7;
    markerIndex [std::make_pair("PLUS", 5)] = SoMarkerSet::PLUS_5_5;

    createBitmap("SQUARE_LINE", 11, 11, 11, squareLine11_marker);
    createBitmap("SQUARE_LINE", 13, 13, 13, squareLine13_marker);
    createBitmap("SQUARE_LINE", 15, 15, 15, squareLine15_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("SQUARE_LINE", 9)] = SoMarkerSet::SQUARE_LINE_9_9;
    markerIndex [std::make_pair("SQUARE_LINE", 7)] = SoMarkerSet::SQUARE_LINE_7_7;
    markerIndex [std::make_pair("SQUARE_LINE", 5)] = SoMarkerSet::SQUARE_LINE_5_5;

    createBitmap("SQUARE_FILLED", 11, 11, 11, squareFilled11_marker);
    createBitmap("SQUARE_FILLED", 13, 13, 13, squareFilled13_marker);
    createBitmap("SQUARE_FILLED", 15, 15, 15, squareFilled15_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("SQUARE_FILLED", 9)] = SoMarkerSet::SQUARE_FILLED_9_9;
    markerIndex [std::make_pair("SQUARE_FILLED", 7)] = SoMarkerSet::SQUARE_FILLED_7_7;
    markerIndex [std::make_pair("SQUARE_FILLED", 5)] = SoMarkerSet::SQUARE_FILLED_5_5;

    createBitmap("CIRCLE_LINE", 11, 11, 11, circleLine11_marker);
    createBitmap("CIRCLE_LINE", 13, 13, 13, circleLine13_marker);
    createBitmap("CIRCLE_LINE", 15, 15, 15, circleLine15_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("CIRCLE_LINE", 9)] = SoMarkerSet::CIRCLE_LINE_9_9;
    markerIndex [std::make_pair("CIRCLE_LINE", 7)] = SoMarkerSet::CIRCLE_LINE_7_7;
    markerIndex [std::make_pair("CIRCLE_LINE", 5)] = SoMarkerSet::CIRCLE_LINE_5_5;

    createBitmap("CIRCLE_FILLED", 11, 11, 11, circleFilled11_marker);
    createBitmap("CIRCLE_FILLED", 13, 13, 13, circleFilled13_marker);
    createBitmap("CIRCLE_FILLED", 15, 15, 15, circleFilled15_marker);

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
    for (const auto & it : markerIndex) {
        if (it.first.first == name)
            sizes.push_back(it.first.second);
    }
    return sizes;
}
