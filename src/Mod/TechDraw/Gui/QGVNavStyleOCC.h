/***************************************************************************
 *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
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


#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include "QGVNavStyle.h"

namespace TechDrawGui {

class QGVPage;

class TechDrawGuiExport QGVNavStyleOCC : public QGVNavStyle
{
public:
    explicit QGVNavStyleOCC(QGVPage* qgvp);
    ~QGVNavStyleOCC() override;

    void handleKeyReleaseEvent(QKeyEvent *event) override;
    void handleMousePressEvent(QMouseEvent *event) override;
    void handleMouseMoveEvent(QMouseEvent *event) override;
    void handleMouseReleaseEvent(QMouseEvent *event) override;

    //context menu (RMB) prevents pan mode 2 (RMB or CNTL+RMB)
    bool allowContextMenu(QContextMenuEvent *event) override;

protected:
private:

};

}