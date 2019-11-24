/***************************************************************************
 *   Copyright (c) 2016 WandererFan   (wandererfan@gmail.com)              *
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

#ifndef DRAWINGGUI_QGCUSTOMIMAGE_H
#define DRAWINGGUI_QGCUSTOMIMAGE_H

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QSize>
#include <QByteArray>
#include <QPixmap>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGCustomImage : public QGraphicsPixmapItem
{
public:
    explicit QGCustomImage(void);
    ~QGCustomImage();

    enum {Type = QGraphicsItem::UserType + 201};
    int type() const override { return Type;}

    virtual void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr ) override;
    virtual void centerAt(QPointF centerPos);
    virtual void centerAt(double cX, double cY);
    virtual bool load(QString fileSpec);
    virtual QSize imageSize(void);

protected:
    QPixmap m_px;

};

} // namespace TechDrawGui

#endif // DRAWINGGUI_QGCUSTOMIMAGE_H
