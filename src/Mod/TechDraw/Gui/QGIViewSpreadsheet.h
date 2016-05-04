/***************************************************************************
 *   Copyright (c) 2016 wandererfan <WandererFan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWSPREADSHEET_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWSPREADSHEET_H

#include <QObject>
#include <QPainter>
#include <QString>
#include <QByteArray>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>

#include "QGIViewSymbol.h"
#include "QGIView.h"

namespace TechDraw {
class DrawViewSpreadsheet;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIViewSpreadsheet : public QGIViewSymbol
{
    Q_OBJECT

public:
    explicit QGIViewSpreadsheet();
    ~QGIViewSpreadsheet();

    enum {Type = QGraphicsItem::UserType + 124};
    int type() const { return Type;}

    //void updateView(bool update = false);
    void setViewFeature(TechDraw::DrawViewSpreadsheet *obj);

protected:
    //void drawSvg();

protected:
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWSPREADSHEET_H
