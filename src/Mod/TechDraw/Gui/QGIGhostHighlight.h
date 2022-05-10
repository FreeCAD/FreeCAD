/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAWGUI_QGIGHOSTHIGHLIGHT_H
#define TECHDRAWGUI_QGIGHOSTHIGHLIGHT_H

#include <QGraphicsScene>
#include <QObject>
#include <QPointF>

#include "QGIHighlight.h"


//a movable, selectable surrogate for detail highlights in QGIVPart

namespace TechDrawGui
{

class TechDrawGuiExport QGIGhostHighlight : public QObject, public QGIHighlight
{
    Q_OBJECT
public:
    explicit QGIGhostHighlight();
    ~QGIGhostHighlight();

    enum {Type = QGraphicsItem::UserType + 177};
    int type() const override { return Type;}

    void setInteractive(bool state);
    void setRadius(double r);

Q_SIGNALS:
    void positionChange(QPointF p);

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    bool m_dragging;

private:
};

}

#endif // TECHDRAWGUI_QGIGHOSTHIGHLIGHT_H
