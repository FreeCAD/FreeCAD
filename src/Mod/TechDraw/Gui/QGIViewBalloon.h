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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>

#include <Base/Vector3D.h>

#include "QGCustomText.h"
#include "QGIView.h"


namespace TechDraw
{
class DrawViewBalloon;
class DrawView;
}// namespace TechDraw

namespace TechDraw
{
class BaseGeom;
class AOC;
}// namespace TechDraw

namespace TechDrawGui
{
class QGIArrow;
class QGIDimLines;
class QGIViewBalloon;

class QGIBalloonLabel: public QGraphicsObject
{
    Q_OBJECT

public:
    QGIBalloonLabel();
    ~QGIBalloonLabel() override = default;

    enum
    {
        Type = QGraphicsItem::UserType + 141
    };
    int type() const override
    {
        return Type;
    }

    QRectF boundingRect() const override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;
    void setLabelCenter();
    void setPosFromCenter(const double& xCenter, const double& yCenter);
    double X() const
    {
        return posX;
    }
    double Y() const
    {
        return posY;
    }//minus posY?

    void setFont(QFont font);
    QFont getFont()
    {
        return m_labelText->font();
    }
    void setDimString(QString text);
    void setDimString(QString text, qreal maxWidth);
    void setPrettySel();
    void setPrettyPre();
    void setPrettyNormal();
    void setColor(QColor color);

    void setQBalloon(QGIViewBalloon* qBalloon)
    {
        parent = qBalloon;
    }


    QGCustomText* getDimText()
    {
        return m_labelText;
    }

    void setDimText(QGCustomText* newText)
    {
        m_labelText = newText;
    }
    bool getVerticalSep() const
    {
        return verticalSep;
    }
    void setVerticalSep(bool sep)
    {
        verticalSep = sep;
    }
    std::vector<int> getSeps() const
    {
        return seps;
    }
    void setSeps(std::vector<int> newSeps)
    {
        seps = newSeps;
    }

Q_SIGNALS:
    void dragging(bool state);
    void hover(bool state);
    void selected(bool state);
    void dragFinished();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    bool hasHover;

    QGIViewBalloon* parent;
    bool verticalSep;
    std::vector<int> seps;

    QGCustomText* m_labelText;
    QColor m_colNormal;

    double posX;
    double posY;
    bool m_ctrl;
    bool m_drag;
};

//*******************************************************************

class TechDrawGuiExport QGIViewBalloon: public QGIView
{
    Q_OBJECT

public:
    enum
    {
        Type = QGraphicsItem::UserType + 140
    };

    explicit QGIViewBalloon();
    ~QGIViewBalloon() override = default;

    void setViewPartFeature(TechDraw::DrawViewBalloon* balloonFeat);
    int type() const override
    {
        return Type;
    }

    void drawBorder() override;
    void updateView(bool update = false) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    QString getLabelText();
    void placeBalloon(QPointF pos);
    void setPrettyPre();
    void setPrettySel();
    void setPrettyNormal();

    void setGroupSelection(bool isSelected) override;
    virtual QGIBalloonLabel* getBalloonLabel()
    {
        return balloonLabel;
    }

    void setNormalColorAll();
    QColor prefNormalColor();
    int prefDefaultArrow() const;
    bool prefOrthoPyramid() const;

    TechDraw::DrawViewBalloon* getBalloonFeat()
    {
        return dvBalloon;
    }

public Q_SLOTS:
    void balloonLabelDragged(bool ctrl);
    void balloonLabelDragFinished();
    void select(bool state);
    void hover(bool state);
    void updateBalloon(bool obtuse = false);

protected:
    void draw() override;
    void drawBalloon(bool dragged = false);
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    virtual void setSvgPens();
    virtual void setPens();
    QString getPrecision();
    void parentViewMousePressed(QGIView* view, QPointF pos);
    TechDraw::DrawView* getSourceView() const;

private:
    TechDraw::DrawViewBalloon* dvBalloon;
    bool hasHover;
    QGIBalloonLabel* balloonLabel;
    QGIDimLines* balloonLines;
    QGIDimLines* balloonShape;
    QGIArrow* arrow;
    double m_lineWidth;
    bool m_obtuse;
    QGIView* parent;//used to create edit dialog

    bool m_dragInProgress;
    bool m_originDragged = false;
    bool m_ctrl;
    Base::Vector3d m_saveOffset;
};

}// namespace TechDrawGui

#endif// TECHDRAWGUI_QGIVBALLOON_H
