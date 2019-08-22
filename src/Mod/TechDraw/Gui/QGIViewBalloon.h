/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2019 Franck Jullien <franck.jullien@gmail.com>          *
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

#ifndef TECHDRAWGUI_QGIVBALLOON_H
#define TECHDRAWGUI_QGIVBALLOON_H

#include <QObject>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QColor>
#include <QFont>
#include <Base/Vector3D.h>
#include "QGIView.h"
#include "QGIViewPart.h"
#include "QGCustomText.h"
#include "QGIViewDimension.h"

namespace TechDraw {
class DrawViewBalloon;
}

namespace TechDraw {
class BaseGeom;
class AOC;
}

namespace TechDrawGui
{
class QGIArrow;
class QGIDimLines;
class QGIViewBalloon;

class QGIBalloonLabel : public QGIDatumLabel
{
Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 141};
    int type() const override { return Type;}

    QGIViewBalloon *parent;

protected:
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};

//*******************************************************************

class TechDrawGuiExport QGIViewBalloon : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 140};

    explicit QGIViewBalloon();
    virtual ~QGIViewBalloon() = default;

    void setViewPartFeature(TechDraw::DrawViewBalloon *obj);
    int type() const override { return Type;}

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;
    virtual QColor getNormalColor(void) override;
    QString getLabelText(void);
    void draw_modifier(bool modifier);
    void placeBalloon(QPointF pos);
    TechDraw::DrawViewBalloon *dvBalloon;

public Q_SLOTS:
    void balloonLabelDragged(bool ctrl);
    void balloonLabelDragFinished(void);
    void select(bool state);
    void hover(bool state);
    void updateBalloon(bool obtuse = false);

protected:
    void draw() override;
    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    virtual void setSvgPens(void);
    virtual void setPens(void);
    QString getPrecision(void);

protected:
    bool hasHover;
    QGIBalloonLabel* balloonLabel;
    QGIDimLines* balloonLines;
    QGIDimLines* balloonShape;
    QGIArrow* arrow;
    double m_lineWidth;
    bool m_obtuse;
    void parentViewMousePressed(QGIView *view, QPointF pos);
    QPointF *oldLabelCenter;
    QGIView *parent;

};

} // namespace

#endif // TECHDRAWGUI_QGIVBALLOON_H
