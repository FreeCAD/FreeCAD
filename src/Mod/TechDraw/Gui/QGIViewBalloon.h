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
class DrawView;
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

class QGIBalloonLabel : public QGraphicsObject
{
Q_OBJECT

public:
    QGIBalloonLabel();
    virtual ~QGIBalloonLabel() = default;

    enum {Type = QGraphicsItem::UserType + 141};
    int type() const override { return Type;}

    virtual QRectF boundingRect() const override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr ) override;
    void setLabelCenter();
    void setPosFromCenter(const double &xCenter, const double &yCenter);
    double X() const { return posX; }
    double Y() const { return posY; }              //minus posY?
    
    void setFont(QFont f);
    QFont getFont(void) { return m_labelText->font(); }
    void setDimString(QString t);
    void setDimString(QString t, qreal maxWidth);
    void setPrettySel(void);
    void setPrettyPre(void);
    void setPrettyNormal(void);
    void setColor(QColor c);

    bool verticalSep;
    std::vector<int> seps;

    QGCustomText* getDimText(void) { return m_labelText; }
    void setDimText(QGCustomText* newText) { m_labelText = newText; }

    bool hasHover;

    QGIViewBalloon *parent;

Q_SIGNALS:
    void dragging(bool);
    void hover(bool state);
    void selected(bool state);
    void dragFinished();

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
//    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event) override;

    QGCustomText* m_labelText;
    QColor m_colNormal;

    double posX;
    double posY;
    bool m_ctrl;
    bool m_drag;
    
private:
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

    QString getLabelText(void);
    void placeBalloon(QPointF pos);
    TechDraw::DrawViewBalloon *dvBalloon;
    void setPrettyPre(void);
    void setPrettySel(void);
    void setPrettyNormal(void);

    virtual void setGroupSelection(bool b) override;
    virtual QGIBalloonLabel* getBalloonLabel(void) { return balloonLabel; }

    virtual QColor getNormalColor(void) override;
    int prefDefaultArrow() const;
    bool prefOrthoPyramid() const;


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
    QGIView *parent;           //used to create edit dialog

    TechDraw::DrawView* getSourceView() const;
    bool m_dragInProgress;
    bool m_ctrl;
    Base::Vector3d m_saveOffset;


};

} // namespace

#endif // TECHDRAWGUI_QGIVBALLOON_H
