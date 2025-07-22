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


//DIAMOND_FILLED_20_20
const int DIAMOND_FILLED20_WIDTH = 20;
const int DIAMOND_FILLED20_HEIGHT = 20;
const char diamondFilled20_marker[DIAMOND_FILLED20_WIDTH * DIAMOND_FILLED20_HEIGHT + 1] = {
"                    "
"          x         "
"         xxx        "
"        xxxxx       "
"       xxxxxxx      "
"      xxxxxxxxx     "
"     xxxxxxxxxxx    "
"    xxxxxxxxxxxxx   "
"   xxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxx"
"  xxxxxxxxxxxxxxxxx "
"   xxxxxxxxxxxxxxx  "
"    xxxxxxxxxxxxx   "
"     xxxxxxxxxxx    "
"      xxxxxxxxx     "
"       xxxxxxx      "
"        xxxxx       "
"         xxx        "
"          x         "};

//DIAMOND_FILLED_25_25
const int DIAMOND_FILLED25_WIDTH = 25;
const int DIAMOND_FILLED25_HEIGHT = 25;
const char diamondFilled25_marker[DIAMOND_FILLED25_WIDTH * DIAMOND_FILLED25_HEIGHT + 1] = {
"                         "
"            xx           "
"           xxxx          "
"          xxxxxx         "
"         xxxxxxxx        "
"        xxxxxxxxxx       "
"       xxxxxxxxxxxx      "
"      xxxxxxxxxxxxxx     "
"     xxxxxxxxxxxxxxxx    "
"    xxxxxxxxxxxxxxxxxx   "
"   xxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
"  xxxxxxxxxxxxxxxxxxxxxx "
"   xxxxxxxxxxxxxxxxxxxx  "
"    xxxxxxxxxxxxxxxxxx   "
"     xxxxxxxxxxxxxxxx    "
"      xxxxxxxxxxxxxx     "
"       xxxxxxxxxxxx      "
"        xxxxxxxxxx       "
"         xxxxxxxx        "
"          xxxxxx         "
"           xxxx          "
"            xx           "};


//DIAMOND_FILLED_30_30
const int DIAMOND_FILLED30_WIDTH = 30;
const int DIAMOND_FILLED30_HEIGHT = 30;
const char diamondFilled30_marker[DIAMOND_FILLED30_WIDTH * DIAMOND_FILLED30_HEIGHT + 1] = {
"                              "
"              xxx             "
"             xxxxx            "
"            xxxxxxx           "
"           xxxxxxxxx          "
"          xxxxxxxxxxx         "
"         xxxxxxxxxxxxx        "
"        xxxxxxxxxxxxxxx       "
"       xxxxxxxxxxxxxxxxx      "
"      xxxxxxxxxxxxxxxxxxx     "
"     xxxxxxxxxxxxxxxxxxxxx    "
"    xxxxxxxxxxxxxxxxxxxxxxx   "
"   xxxxxxxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
"  xxxxxxxxxxxxxxxxxxxxxxxxxxx "
"   xxxxxxxxxxxxxxxxxxxxxxxxx  "
"    xxxxxxxxxxxxxxxxxxxxxxx   "
"     xxxxxxxxxxxxxxxxxxxxx    "
"      xxxxxxxxxxxxxxxxxxx     "
"       xxxxxxxxxxxxxxxxx      "
"        xxxxxxxxxxxxxxx       "
"         xxxxxxxxxxxxx        "
"          xxxxxxxxxxx         "
"           xxxxxxxxx          "
"            xxxxxxx           "
"             xxxxx            "
"              xxx             "
"                              "};


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

//CROSS_20_20
const int CROSS20_WIDTH = 20;
const int CROSS20_HEIGHT = 20;
const char cross20_marker[CROSS20_WIDTH * CROSS20_HEIGHT + 1] = {
"                    "
" xx               xx"
"  xx             xx "
"   xx           xx  "
"    xx         xx   "
"     xx       xx    "
"      xx     xx     "
"       xx   xx      "
"        xx xx       "
"         xxx        "
"          x         "
"         xxx        "
"        xx xx       "
"       xx   xx      "
"      xx     xx     "
"     xx       xx    "
"    xx         xx   "
"   xx           xx  "
"  xx             xx "
" xx               xx"};


//CROSS_25_25
const int CROSS25_WIDTH = 25;
const int CROSS25_HEIGHT = 25;
const char cross25_marker[CROSS25_WIDTH * CROSS25_HEIGHT + 1] = {
"                         "
" xx                    xx"
"  xx                  xx "
"   xx                xx  "
"    xx              xx   "
"     xx            xx    "
"      xx          xx     "
"       xx        xx      "
"        xx      xx       "
"         xx    xx        "
"          xx  xx         "
"           xxxx          "
"            xx           "
"            xx           "
"           xxxx          "
"          xx  xx         "
"         xx    xx        "
"        xx      xx       "
"       xx        xx      "
"      xx          xx     "
"     xx            xx    "
"    xx              xx   "
"   xx                xx  "
"  xx                  xx "
" xx                    xx"};


//CROSS_30_30
const int CROSS30_WIDTH = 30;
const int CROSS30_HEIGHT = 30;
const char cross30_marker[CROSS30_WIDTH * CROSS30_HEIGHT + 1] = {
"                              "
" xxx                       xxx"
"  xxx                     xxx "
"   xxx                   xxx  "
"    xxx                 xxx   "
"     xxx               xxx    "
"      xxx             xxx     "
"       xxx           xxx      "
"        xxx         xxx       "
"         xxx       xxx        "
"          xxx     xxx         "
"           xxx   xxx          "
"            xxx xxx           "
"             xxxxx            "
"              xxx             "
"              xxx             "
"              xxx             "
"             xxxxx            "
"            xxx xxx           "
"           xxx   xxx          "
"          xxx     xxx         "
"         xxx       xxx        "
"        xxx         xxx       "
"       xxx           xxx      "
"      xxx             xxx     "
"     xxx               xxx    "
"    xxx                 xxx   "
"   xxx                   xxx  "
"  xxx                     xxx "
" xxx                       xxx"};



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


//PLUS_20_20
const int PLUS20_WIDTH = 20;
const int PLUS20_HEIGHT = 20;
const char plus20_marker[PLUS20_WIDTH * PLUS20_HEIGHT + 1] = {
    "                    "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    " xxxxxxxxxxxxxxxxxxx"
    " xxxxxxxxxxxxxxxxxxx"
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "
    "          xx        "};

//PLUS_25_25
const int PLUS25_WIDTH = 25;
const int PLUS25_HEIGHT = 25;
const char plus25_marker[PLUS25_WIDTH * PLUS25_HEIGHT + 1] = {
    "                         "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    " xxxxxxxxxxxxxxxxxxxxxxxx"
    " xxxxxxxxxxxxxxxxxxxxxxxx"
    " xxxxxxxxxxxxxxxxxxxxxxxx"
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "
    "            xxx          "};

//PLUS_30_30
const int PLUS30_WIDTH = 30;
const int PLUS30_HEIGHT = 30;
const char plus30_marker[PLUS30_WIDTH * PLUS30_HEIGHT + 1] = {
    "                              "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    " xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    " xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    " xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "
    "              xxx             "};

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

//SQUARE_LINE_20_20
const int SQUARE_LINE20_WIDTH = 20;
const int SQUARE_LINE20_HEIGHT = 20;
const char squareLine20_marker[SQUARE_LINE20_WIDTH * SQUARE_LINE20_HEIGHT + 1] = {
"                    "
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xx               xx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"};

//SQUARE_LINE_25_25
const int SQUARE_LINE25_WIDTH = 25;
const int SQUARE_LINE25_HEIGHT = 25;
const char squareLine25_marker[SQUARE_LINE25_WIDTH * SQUARE_LINE25_HEIGHT + 1] = {
"                         "
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xx                    xx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"};

//SQUARE_LINE_30_30
const int SQUARE_LINE30_WIDTH = 30;
const int SQUARE_LINE30_HEIGHT = 30;
const char squareLine30_marker[SQUARE_LINE30_WIDTH * SQUARE_LINE30_HEIGHT + 1] = {
"                              "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xx                         xx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"};


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

//SQUARE_FILLED_20_20
const int SQUARE_FILLED20_WIDTH = 20;
const int SQUARE_FILLED20_HEIGHT = 20;
const char squareFilled20_marker[SQUARE_FILLED20_WIDTH * SQUARE_FILLED20_HEIGHT + 1] = {
"                    "
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"};


//SQUARE_FILLED_25_25
const int SQUARE_FILLED25_WIDTH = 25;
const int SQUARE_FILLED25_HEIGHT = 25;
const char squareFilled25_marker[SQUARE_FILLED25_WIDTH * SQUARE_FILLED25_HEIGHT + 1] = {
"                         "
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxx"};


//SQUARE_FILLED_30_30
const int SQUARE_FILLED30_WIDTH = 30;
const int SQUARE_FILLED30_HEIGHT = 30;
const char squareFilled30_marker[SQUARE_FILLED30_WIDTH * SQUARE_FILLED30_HEIGHT + 1] = {
"                              "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"};



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

//CIRCLE_LINE_20_20
const int CIRCLE_LINE20_WIDTH = 20;
const int CIRCLE_LINE20_HEIGHT = 20;
const char circleLine20_marker[CIRCLE_LINE20_WIDTH * CIRCLE_LINE20_HEIGHT + 1] = {
"                    "
"        xxxxx       "
"      xxxxxxxxx     "
"     xx       xx    "
"    xx         xx   "
"   xx           xx  "
"  xx             xx "
"  xx             xx "
"  xx             xx "
" xx               xx"
" xx               xx"
" xx               xx"
"  xx             xx "
"  xx             xx "
"  xx             xx "
"   xx           xx  "
"    xx         xx   "
"     xx       xx    "
"      xxxxxxxxx     "
"        xxxxx       "};



//CIRCLE_LINE_25_25
const int CIRCLE_LINE25_WIDTH = 25;
const int CIRCLE_LINE25_HEIGHT = 25;
const char circleLine25_marker[CIRCLE_LINE25_WIDTH * CIRCLE_LINE25_HEIGHT + 1] = {
"                         "
"         xxxxxxx         "
"       xxxxxxxxxxx       "
"     xxx         xxx     "
"    xx             xx    "
"   xx               xx   "
"  xx                 xx  "
"  xx                 xx  "
" xx                   xx "
" xx                   xx "
" xx                   xx "
"xx                     xx"
"xx                     xx"
"xx                     xx"
"xx                     xx"
" xx                   xx "
" xx                   xx "
" xx                   xx "
"  xx                 xx  "
"  xx                 xx  "
"   xx               xx   "
"    xx             xx    "
"     xxx         xxx     "
"       xxxxxxxxxxx       "
"         xxxxxxx         "};




//CIRCLE_LINE_30_30
const int CIRCLE_LINE30_WIDTH = 30;
const int CIRCLE_LINE30_HEIGHT = 30;
const char circleLine30_marker[CIRCLE_LINE30_WIDTH * CIRCLE_LINE30_HEIGHT + 1] = {
"                              "
"            xxxxxx            "
"         xxxxxxxxxxxx         "
"       xxx          xxx       "
"      xx              xx      "
"    xx                  xx    "
"    xx                  xx    "
"   xx                    xx   "
"  xx                      xx  "
"  xx                      xx  "
" xx                        xx "
" xx                        xx "
" xx                        xx "
"xx                          xx"
"xx                          xx"
"xx                          xx"
"xx                          xx"
"xx                          xx"
" xx                        xx "
" xx                        xx "
" xx                        xx "
"  xx                      xx  "
"  xx                      xx  "
"   xx                    xx   "
"    xx                  xx    "
"    xx                  xx    "
"      xx              xx      "
"       xxx          xxx       "
"         xxxxxxxxxxxx         "
"            xxxxxx            "};



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


//CIRCLE_FILLED_20_20
const int CIRCLE_FILLED20_WIDTH = 20;
const int CIRCLE_FILLED20_HEIGHT = 20;
const char circleFilled20_marker[CIRCLE_FILLED20_WIDTH * CIRCLE_FILLED20_HEIGHT + 1] = {
"                    "
"        xxxxx       "
"      xxxxxxxxx     "
"     xxxxxxxxxxx    "
"    xxxxxxxxxxxxx   "
"   xxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxx "
"  xxxxxxxxxxxxxxxxx "
"  xxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxx"
"  xxxxxxxxxxxxxxxxx "
"  xxxxxxxxxxxxxxxxx "
"  xxxxxxxxxxxxxxxxx "
"   xxxxxxxxxxxxxxx  "
"    xxxxxxxxxxxxx   "
"     xxxxxxxxxxx    "
"      xxxxxxxxx     "
"        xxxxx       "};


//CIRCLE_FILLED_25_25
const int CIRCLE_FILLED25_WIDTH = 25;
const int CIRCLE_FILLED25_HEIGHT = 25;
const char circleFilled25_marker[CIRCLE_FILLED25_WIDTH * CIRCLE_FILLED25_HEIGHT + 1] = {
"                         "
"         xxxxxxx         "
"       xxxxxxxxxxx       "
"     xxxxxxxxxxxxxxx     "
"    xxxxxxxxxxxxxxxxx    "
"   xxxxxxxxxxxxxxxxxxx   "
"  xxxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxx  "
" xxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxx "
"xxxxxxxxxxxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxx "
"  xxxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxx  "
"   xxxxxxxxxxxxxxxxxxx   "
"    xxxxxxxxxxxxxxxxx    "
"     xxxxxxxxxxxxxxx     "
"       xxxxxxxxxxx       "
"         xxxxxxx         "};


//CIRCLE_FILLED_30_30
const int CIRCLE_FILLED30_WIDTH = 30;
const int CIRCLE_FILLED30_HEIGHT = 30;
const char circleFilled30_marker[CIRCLE_FILLED30_WIDTH * CIRCLE_FILLED30_HEIGHT + 1] = {
"                              "
"            xxxxxx            "
"         xxxxxxxxxxxx         "
"       xxxxxxxxxxxxxxxx       "
"      xxxxxxxxxxxxxxxxxx      "
"    xxxxxxxxxxxxxxxxxxxxxx    "
"    xxxxxxxxxxxxxxxxxxxxxx    "
"   xxxxxxxxxxxxxxxxxxxxxxxx   "
"  xxxxxxxxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxxxxxxx  "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxx "
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
" xxxxxxxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxx "
"  xxxxxxxxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxxxxxxx  "
"   xxxxxxxxxxxxxxxxxxxxxxxx   "
"    xxxxxxxxxxxxxxxxxxxxxx    "
"    xxxxxxxxxxxxxxxxxxxxxx    "
"      xxxxxxxxxxxxxxxxxx      "
"       xxxxxxxxxxxxxxxx       "
"         xxxxxxxxxxxx         "
"            xxxxxx            "};

//HOURGLASS_FILLED_11_11
const int HOURGLASS_FILLED11_WIDTH = 11;
const int HOURGLASS_FILLED11_HEIGHT = 11;
const char hourglassFilled11_marker[HOURGLASS_FILLED11_WIDTH * HOURGLASS_FILLED11_HEIGHT + 1] = {
"           "
" xxxxxxxxxx"
"  xxxxxxxx "
"   xxxxxx  "
"    xxxx   "
"     xx    "
"     xx    "
"    xxxx   "
"   xxxxxx  "
"  xxxxxxxx "
" xxxxxxxxxx"};


//HOURGLASS_FILLED_13_13
const int HOURGLASS_FILLED13_WIDTH = 13;
const int HOURGLASS_FILLED13_HEIGHT = 13;
const char hourglassFilled13_marker[HOURGLASS_FILLED13_WIDTH * HOURGLASS_FILLED13_HEIGHT + 1] = {
"             "
" xxxxxxxxxxxx"
"  xxxxxxxxxx "
"   xxxxxxxx  "
"    xxxxxx   "
"     xxxx    "
"      xx     "
"      xx     "
"     xxxx    "
"    xxxxxx   "
"   xxxxxxxx  "
"  xxxxxxxxxx "
" xxxxxxxxxxxx"};


//HOURGLASS_FILLED_15_15
const int HOURGLASS_FILLED15_WIDTH = 15;
const int HOURGLASS_FILLED15_HEIGHT = 15;
const char hourglassFilled15_marker[HOURGLASS_FILLED15_WIDTH * HOURGLASS_FILLED15_HEIGHT + 1] = {
"               "
" xxxxxxxxxxxxxx"
"  xxxxxxxxxxxx "
"   xxxxxxxxxx  "
"    xxxxxxxx   "
"     xxxxxx    "
"      xxxx     "
"       xx      "
"       xx      "
"      xxxx     "
"     xxxxxx    "
"    xxxxxxxx   "
"   xxxxxxxxxx  "
"  xxxxxxxxxxxx "
" xxxxxxxxxxxxxx"};

//HOURGLASS_FILLED_20_20
const int HOURGLASS_FILLED20_WIDTH = 20;
const int HOURGLASS_FILLED20_HEIGHT = 20;
const char hourglassFilled20_marker[HOURGLASS_FILLED20_WIDTH * HOURGLASS_FILLED20_HEIGHT + 1] = {
"                    "
" xxxxxxxxxxxxxxxxxxx"
"  xxxxxxxxxxxxxxxxx "
"   xxxxxxxxxxxxxxx  "
"    xxxxxxxxxxxxx   "
"     xxxxxxxxxxx    "
"      xxxxxxxxx     "
"       xxxxxxx      "
"        xxxxx       "
"         xxx        "
"          x         "
"         xxx        "
"        xxxxx       "
"       xxxxxxx      "
"      xxxxxxxxx     "
"     xxxxxxxxxxx    "
"    xxxxxxxxxxxxx   "
"   xxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxx"};


//HOURGLASS_FILLED_25_25
const int HOURGLASS_FILLED25_WIDTH = 25;
const int HOURGLASS_FILLED25_HEIGHT = 25;
const char hourglassFilled25_marker[HOURGLASS_FILLED25_WIDTH * HOURGLASS_FILLED25_HEIGHT + 1] = {
"                         "
" xxxxxxxxxxxxxxxxxxxxxxxx"
"  xxxxxxxxxxxxxxxxxxxxxx "
"   xxxxxxxxxxxxxxxxxxxx  "
"    xxxxxxxxxxxxxxxxxx   "
"     xxxxxxxxxxxxxxxx    "
"      xxxxxxxxxxxxxx     "
"       xxxxxxxxxxxx      "
"        xxxxxxxxxx       "
"         xxxxxxxx        "
"          xxxxxx         "
"           xxxx          "
"            xx           "
"            xx           "
"           xxxx          "
"          xxxxxx         "
"         xxxxxxxx        "
"        xxxxxxxxxx       "
"       xxxxxxxxxxxx      "
"      xxxxxxxxxxxxxx     "
"     xxxxxxxxxxxxxxxx    "
"    xxxxxxxxxxxxxxxxxx   "
"   xxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxx"};


//HOURGLASS_FILLED_30_30
const int HOURGLASS_FILLED30_WIDTH = 30;
const int HOURGLASS_FILLED30_HEIGHT = 30;
const char hourglassFilled30_marker[HOURGLASS_FILLED30_WIDTH * HOURGLASS_FILLED30_HEIGHT + 1] = {
"                              "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
"  xxxxxxxxxxxxxxxxxxxxxxxxxxx "
"   xxxxxxxxxxxxxxxxxxxxxxxxx  "
"    xxxxxxxxxxxxxxxxxxxxxxx   "
"     xxxxxxxxxxxxxxxxxxxxx    "
"      xxxxxxxxxxxxxxxxxxx     "
"       xxxxxxxxxxxxxxxxx      "
"        xxxxxxxxxxxxxxx       "
"         xxxxxxxxxxxxx        "
"          xxxxxxxxxxx         "
"           xxxxxxxxx          "
"            xxxxxxx           "
"             xxxxx            "
"              xxx             "
"              xxx             "
"              xxx             "
"             xxxxx            "
"            xxxxxxx           "
"           xxxxxxxxx          "
"          xxxxxxxxxxx         "
"         xxxxxxxxxxxxx        "
"        xxxxxxxxxxxxxxx       "
"       xxxxxxxxxxxxxxxxx      "
"      xxxxxxxxxxxxxxxxxxx     "
"     xxxxxxxxxxxxxxxxxxxxx    "
"    xxxxxxxxxxxxxxxxxxxxxxx   "
"   xxxxxxxxxxxxxxxxxxxxxxxxx  "
"  xxxxxxxxxxxxxxxxxxxxxxxxxxx "
" xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"};


std::map<MarkerBitmaps::Marker, int> MarkerBitmaps::markerIndex;

void
MarkerBitmaps::initClass()
{
    createBitmap("DIAMOND_FILLED", 11, 11, 11, diamondFilled11_marker);
    createBitmap("DIAMOND_FILLED", 13, 13, 13, diamondFilled13_marker);
    createBitmap("DIAMOND_FILLED", 15, 15, 15, diamondFilled15_marker);
    createBitmap("DIAMOND_FILLED", 20, 20, 20, diamondFilled20_marker);
    createBitmap("DIAMOND_FILLED", 25, 25, 25, diamondFilled25_marker);
    createBitmap("DIAMOND_FILLED", 30, 30, 30, diamondFilled30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("DIAMOND_FILLED", 9)] = SoMarkerSet::DIAMOND_FILLED_9_9;
    markerIndex [std::make_pair("DIAMOND_FILLED", 7)] = SoMarkerSet::DIAMOND_FILLED_7_7;
    markerIndex [std::make_pair("DIAMOND_FILLED", 5)] = SoMarkerSet::DIAMOND_FILLED_5_5;

    createBitmap("CROSS", 11, 11, 11, cross11_marker);
    createBitmap("CROSS", 13, 13, 13, cross13_marker);
    createBitmap("CROSS", 15, 15, 15, cross15_marker);
    createBitmap("CROSS", 20, 20, 20, cross20_marker);
    createBitmap("CROSS", 25, 25, 25, cross25_marker);
    createBitmap("CROSS", 30, 30, 30, cross30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("CROSS", 9)] = SoMarkerSet::CROSS_9_9;
    markerIndex [std::make_pair("CROSS", 7)] = SoMarkerSet::CROSS_7_7;
    markerIndex [std::make_pair("CROSS", 5)] = SoMarkerSet::CROSS_5_5;

    createBitmap("PLUS", 11, 11, 11, plus11_marker);
    createBitmap("PLUS", 13, 13, 13, plus13_marker);
    createBitmap("PLUS", 15, 15, 15, plus15_marker);
    createBitmap("PLUS", 20, 20, 20, plus20_marker);
    createBitmap("PLUS", 25, 25, 25, plus25_marker);
    createBitmap("PLUS", 30, 30, 30, plus30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("PLUS", 9)] = SoMarkerSet::PLUS_9_9;
    markerIndex [std::make_pair("PLUS", 7)] = SoMarkerSet::PLUS_7_7;
    markerIndex [std::make_pair("PLUS", 5)] = SoMarkerSet::PLUS_5_5;

    createBitmap("SQUARE_LINE", 11, 11, 11, squareLine11_marker);
    createBitmap("SQUARE_LINE", 13, 13, 13, squareLine13_marker);
    createBitmap("SQUARE_LINE", 15, 15, 15, squareLine15_marker);
    createBitmap("SQUARE_LINE", 20, 20, 20, squareLine20_marker);
    createBitmap("SQUARE_LINE", 25, 25, 25, squareLine25_marker);
    createBitmap("SQUARE_LINE", 30, 30, 30, squareLine30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("SQUARE_LINE", 9)] = SoMarkerSet::SQUARE_LINE_9_9;
    markerIndex [std::make_pair("SQUARE_LINE", 7)] = SoMarkerSet::SQUARE_LINE_7_7;
    markerIndex [std::make_pair("SQUARE_LINE", 5)] = SoMarkerSet::SQUARE_LINE_5_5;

    createBitmap("SQUARE_FILLED", 11, 11, 11, squareFilled11_marker);
    createBitmap("SQUARE_FILLED", 13, 13, 13, squareFilled13_marker);
    createBitmap("SQUARE_FILLED", 15, 15, 15, squareFilled15_marker);
    createBitmap("SQUARE_FILLED", 20, 20, 20, squareFilled20_marker);
    createBitmap("SQUARE_FILLED", 25, 25, 25, squareFilled25_marker);
    createBitmap("SQUARE_FILLED", 30, 30, 30, squareFilled30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("SQUARE_FILLED", 9)] = SoMarkerSet::SQUARE_FILLED_9_9;
    markerIndex [std::make_pair("SQUARE_FILLED", 7)] = SoMarkerSet::SQUARE_FILLED_7_7;
    markerIndex [std::make_pair("SQUARE_FILLED", 5)] = SoMarkerSet::SQUARE_FILLED_5_5;

    createBitmap("CIRCLE_LINE", 11, 11, 11, circleLine11_marker);
    createBitmap("CIRCLE_LINE", 13, 13, 13, circleLine13_marker);
    createBitmap("CIRCLE_LINE", 15, 15, 15, circleLine15_marker);
    createBitmap("CIRCLE_LINE", 20, 20, 20, circleLine20_marker);
    createBitmap("CIRCLE_LINE", 25, 25, 25, circleLine25_marker);
    createBitmap("CIRCLE_LINE", 30, 30, 30, circleLine30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("CIRCLE_LINE", 9)] = SoMarkerSet::CIRCLE_LINE_9_9;
    markerIndex [std::make_pair("CIRCLE_LINE", 7)] = SoMarkerSet::CIRCLE_LINE_7_7;
    markerIndex [std::make_pair("CIRCLE_LINE", 5)] = SoMarkerSet::CIRCLE_LINE_5_5;

    createBitmap("CIRCLE_FILLED", 11, 11, 11, circleFilled11_marker);
    createBitmap("CIRCLE_FILLED", 13, 13, 13, circleFilled13_marker);
    createBitmap("CIRCLE_FILLED", 15, 15, 15, circleFilled15_marker);
    createBitmap("CIRCLE_FILLED", 20, 20, 20, circleFilled20_marker);
    createBitmap("CIRCLE_FILLED", 25, 25, 25, circleFilled25_marker);
    createBitmap("CIRCLE_FILLED", 30, 30, 30, circleFilled30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("CIRCLE_FILLED", 9)] = SoMarkerSet::CIRCLE_FILLED_9_9;
    markerIndex [std::make_pair("CIRCLE_FILLED", 7)] = SoMarkerSet::CIRCLE_FILLED_7_7;
    markerIndex [std::make_pair("CIRCLE_FILLED", 5)] = SoMarkerSet::CIRCLE_FILLED_5_5;

    createBitmap("HOURGLASS_FILLED", 11, 11, 11, hourglassFilled11_marker);
    createBitmap("HOURGLASS_FILLED", 13, 13, 13, hourglassFilled13_marker);
    createBitmap("HOURGLASS_FILLED", 15, 15, 15, hourglassFilled15_marker);
    createBitmap("HOURGLASS_FILLED", 20, 20, 20, hourglassFilled20_marker);
    createBitmap("HOURGLASS_FILLED", 25, 25, 25, hourglassFilled25_marker);
    createBitmap("HOURGLASS_FILLED", 30, 30, 30, hourglassFilled30_marker);

    // the built-in bitmaps of Coin
    markerIndex [std::make_pair("HOURGLASS_FILLED", 9)] = SoMarkerSet::HOURGLASS_FILLED_9_9;
    markerIndex [std::make_pair("HOURGLASS_FILLED", 7)] = SoMarkerSet::HOURGLASS_FILLED_7_7;
    markerIndex [std::make_pair("HOURGLASS_FILLED", 5)] = SoMarkerSet::HOURGLASS_FILLED_5_5;
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
        if (it.first.first == name) {
            sizes.push_back(it.first.second);
        }
    }
    return sizes;
}
