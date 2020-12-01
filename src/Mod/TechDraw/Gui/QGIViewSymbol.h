/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2014 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWSYMBOL_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWSYMBOL_H

#include <QObject>
#include <QPainter>
#include <QString>
#include <QByteArray>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>

#include "QGIView.h"

namespace TechDraw {
class DrawViewSymbol;
}

namespace TechDrawGui
{
class QGCustomSvg;
class QGDisplayArea;

class TechDrawGuiExport QGIViewSymbol : public QGIView
{
public:
    QGIViewSymbol();
    ~QGIViewSymbol();

    enum {Type = QGraphicsItem::UserType + 121};
    int type() const override { return Type;}

    virtual void updateView(bool update = false) override;
    void setViewSymbolFeature(TechDraw::DrawViewSymbol *obj);

    virtual void draw() override;
    virtual void rotateView(void) override;


protected:
    virtual void drawSvg();
    void symbolToSvg(QByteArray qba);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    QGDisplayArea* m_displayArea;
    QGCustomSvg *m_svgItem;
};

} // namespace
#endif // DRAWINGGUI_QGRAPHICSITEMVIEWSYMBOL_H
