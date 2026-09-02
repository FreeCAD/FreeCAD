// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#include "Dummy3DViewer.h"

using namespace Gui;

namespace CAMSimulator
{

Dummy3DViewer::Dummy3DViewer(QWidget* parent)
    : View3DInventorViewer(parent)
{
    addViewProvider(&stockViewProvider);
    addViewProvider(&baseViewProvider);
}

void Dummy3DViewer::cloneFrom(Dummy3DViewer& viewer)
{
    // move view providers from viewer to us

    stockViewProvider = std::move(viewer.stockViewProvider);
    baseViewProvider = std::move(viewer.baseViewProvider);
}

void Dummy3DViewer::setStockShape(const Part::TopoShape& shape)
{
    stockViewProvider.setShape(shape);
}

void Dummy3DViewer::setStockVisible(bool b)
{
    stockViewProvider.setShapeVisible(b);
}

void Dummy3DViewer::setBaseShape(const Part::TopoShape& shape)
{
    baseViewProvider.setShape(shape);
}

void Dummy3DViewer::setBaseVisible(bool b)
{
    baseViewProvider.setShapeVisible(b);
}

void Dummy3DViewer::paintEvent(QPaintEvent* event)
{
    if (discardPaintEvent_) {
        return;
    }

    View3DInventorViewer::paintEvent(event);
}

}  // namespace CAMSimulator
