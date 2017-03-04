/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWDIMENSION_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWDIMENSION_H

#include <QObject>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QColor>
#include <Base/Vector3D.h>
#include "QGIView.h"
#include "QGCustomText.h"

namespace TechDraw {
class DrawViewDimension;
}

namespace TechDrawGeometry {
class BaseGeom;
class AOC;
}

namespace TechDrawGui
{
class QGIArrow;
class QGIDimLines;

class QGIDatumLabel : public QGCustomText
{
Q_OBJECT

public:
    explicit QGIDatumLabel();
    ~QGIDatumLabel() {}

    enum {Type = QGraphicsItem::UserType + 107};
    int type() const { return Type;}

    void setLabelCenter();
    void setPosFromCenter(const double &xCenter, const double &yCenter);
    double X() const { return posX; }
    double Y() const { return posY; }

Q_SIGNALS:
    void dragging();
    void hover(bool state);
    void selected(bool state);
    void dragFinished();

protected:
    // Preselection events:
    void mouseReleaseEvent( QGraphicsSceneMouseEvent * event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    // Selection detection
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    double posX;
    double posY;

private:
};

class TechDrawGuiExport QGIViewDimension : public QObject, public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 106};

    explicit QGIViewDimension();
    ~QGIViewDimension() = default;

    void setViewPartFeature(TechDraw::DrawViewDimension *obj);
    int type() const override { return Type;}

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;
    virtual QColor getNormalColor(void) override;

public Q_SLOTS:
    void datumLabelDragged(void);
    void datumLabelDragFinished(void);
    void select(bool state);
    void hover(bool state);
    void updateDim(void);

protected:
    void draw() override;
    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    virtual void setSvgPens(void);
    virtual void setPens(void);

protected:
    bool hasHover;
    QGIDatumLabel* datumLabel;                                         //dimension text
    QGIDimLines* dimLines;                                       //dimension lines + extension lines
    QGIArrow* aHead1;
    QGIArrow* aHead2;
    //QGICMark* centerMark
    double m_lineWidth;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWDIMENSION_H
