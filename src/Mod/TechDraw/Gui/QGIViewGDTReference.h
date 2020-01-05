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

#ifndef TECHDRAWGUI_QGIVIEWGDTREFERENCE_H
#define TECHDRAWGUI_QGIVIEWGDTREFERENCE_H

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
#include <Mod/TechDraw/App/DrawViewGDTReference.h>

namespace TechDraw {
class DrawViewGDTReference;
}

namespace TechDraw {
class BaseGeom;
class AOC;
}

namespace TechDrawGui
{
class QGIArrow;
class QGIDimLines;
class QGIViewGDTReference;

class QGIReferenceLabel : public QGraphicsObject
{
Q_OBJECT

public:
	QGIReferenceLabel(QGIViewGDTReference* parent);
    virtual ~QGIReferenceLabel() = default;

    enum {Type = QGraphicsItem::UserType + 146};
    int type() const override { return Type;}

    virtual QRectF boundingRect() const override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void paint( QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget = nullptr ) override;
    void setLabelCenter();
    void setPosFromCenter(const double &xCenter, const double &yCenter);
    double X() const { return posX; }
    double Y() const { return posY; }
    
    double marginHeight() const { return m_marginHeight; }
    double marginWidth() const { return m_marginWidth; }

    void setFont(QFont f);
    QFont getFont(void) { return m_labelText->font(); }
    void setPrettySel(void);
    void setPrettyPre(void);
    void setPrettyNormal(void);
    void setColor(QColor c);

    void setLabelString(QString t);
    void setLabelString(QString t, qreal maxWidth);
    QGCustomText* getLabelText(void) { return m_labelText; }
    void setLabelText(QGCustomText* newText) { m_labelText = newText; }

    bool hasHover;
    Base::Vector3d linkDir;
    QGIViewGDTReference *parent;

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
// TODO    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event) override;

    QGCustomText* m_labelText;
    QColor m_colNormal;
    bool m_ctrl;

    double posX;
    double posY;
    double m_marginHeight;
    double m_marginWidth;

    
private:
};

//*******************************************************************
// QGIViewGDTReference
//*******************************************************************
class TechDrawGuiExport QGIViewGDTReference : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 145};

    explicit QGIViewGDTReference();
    virtual ~QGIViewGDTReference() = default;

    void setViewPartFeature(TechDraw::DrawViewGDTReference *obj);
    int type() const override { return Type;}

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;
    virtual QColor getNormalColor(void) override;
    QString getLabelText(void);
    void draw_modifier(bool modifier);
    TechDraw::DrawViewGDTReference *dvReference;

public Q_SLOTS:
    void referenceLabelDragged(bool ctrl);
    void referenceLabelDragFinished(void);
    void select(bool state);
    void hover(bool state);
    void updateReference(void);

protected:
    void draw() override;
    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    virtual void setSvgPens(void);
    virtual void setPens(void);
    QString getPrecision(void);

protected:
    bool hasHover;
    QGIReferenceLabel* referenceLabel;
    QGIDimLines* referenceLines;
    QGIDimLines* referenceShape;
    QGIArrow* referenceArrow;
    double m_lineWidth;
    void parentViewMousePressed(QGIView *view, QPointF pos);
    QGIView *parent;
    double labelAngle;

private:
    Base::Vector3d calculateCenter(TechDraw::PointPair & segment);
    Base::Vector3d calculateLabelPlacement(TechDraw::PointPair & segment, Base::Vector3d & origin, double length);
    //static inline Base::Vector2d fromQtApp(const Base::Vector3d &v) { return Base::Vector2d(v.x, -v.y); }
    // TODO move to utils (using in QGIViewDimension)
    double getIsoStandardLinePlacement(double labelAngle);
    static inline double toDeg(double a) { return a*180/M_PI; }
    static inline Base::Vector2d fromQtApp(const Base::Vector3d &v) { return Base::Vector2d(v.x, -v.y); }
    static inline double toQtDeg(double a) { return -a*180.0/M_PI; }
};

} // namespace

#endif // TECHDRAWGUI_QGIVIEWGDTREFERENCE_H
