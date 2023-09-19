/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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

#ifndef DRAWINGGUI_PRECOMPILED_H
#define DRAWINGGUI_PRECOMPILED_H

#include <FCConfig.h>

#ifdef _MSC_VER
#pragma warning(disable : 4005)
#endif

#ifdef _PreComp_

// STL
#include <cmath>
#include <sstream>
#include <vector>

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGLWidget>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QRegExp>
#include <QTimer>
#include <QWheelEvent>

#endif  //_PreComp_

#endif  // DRAWINGGUI_PRECOMPILED_H
