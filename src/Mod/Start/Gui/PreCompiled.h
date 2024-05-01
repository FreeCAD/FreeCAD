// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
#    Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef STARTGUI_PRECOMPILED_H
#define STARTGUI_PRECOMPILED_H

#include <FCConfig.h>

#ifdef _MSC_VER
#pragma warning(disable : 5208)
#endif

#ifdef _PreComp_

// standard
#include <cinttypes>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

// Qt
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFile>
#include <QFileIconProvider>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSpacerItem>
#include <QString>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#endif  // _PreComp_
#endif  // STARTGUI_PRECOMPILED_H
