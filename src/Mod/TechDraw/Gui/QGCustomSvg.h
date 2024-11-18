/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGCUSTOMSVG_H
#define DRAWINGGUI_QGCUSTOMSVG_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QByteArray>
#include <QGraphicsItem>
#include <QGraphicsSvgItem>
#include <QPointF>
#include <QSvgRenderer>


QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGCustomSvg : public QGraphicsSvgItem
{
public:
    explicit QGCustomSvg();
    ~QGCustomSvg() override;

    enum {Type = QGraphicsItem::UserType + 131};
    int type() const override { return Type;}

    void paint( QPainter *painter,
                const QStyleOptionGraphicsItem *option,
                QWidget *widget = nullptr ) override;
    virtual void centerAt(QPointF centerPos);
    virtual void centerAt(double cX, double cY);
    virtual bool load(QByteArray *svgString);
    virtual bool load(QString filename);

protected:
    QSvgRenderer *m_svgRender;
};

} // namespace TechDrawGui

#endif // DRAWINGGUI_QGCUSTOMSVG_H
