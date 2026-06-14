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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>

#include <Base/Vector3D.h>

#include "QGCustomText.h"
#include "QGIView.h"
#include "QGIUserTypes.h"


namespace TechDraw
{
class DrawViewBalloon;
class DrawView;
enum class ArrowType : int;
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

    enum {Type = UserType::QGIBalloonLabel};
    int type() const override
    {
        return Type;
    }

    QRectF boundingRect() const override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;
    void setLabelCenter();
    Base::Vector3d getLabelCenter() const;
    void setPosFromCenter(const double& xCenter, const double& yCenter);

    double getCenterX() const
    {
        return mapToParent(m_labelText->boundingRect().center()).x();
    }
    double getCenterY() const
    {
        return mapToParent(m_labelText->boundingRect().center()).y();
    }

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
        newText->setTightBounding(true);
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
    QGCustomText* m_labelText;

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

    QColor m_colNormal;

    bool m_originDrag;
    bool m_dragging;
};

//*******************************************************************

class TechDrawGuiExport QGIViewBalloon: public QGIView
{
    Q_OBJECT

public:
    enum {Type = UserType::QGIViewBalloon};

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

    bool getGroupSelection() override;
    void setGroupSelection(bool isSelected) override;
    virtual QGIBalloonLabel* getBalloonLabel()
    {
        return balloonLabel;
    }

    void setNormalColorAll();
    QColor prefNormalColor();
    TechDraw::ArrowType prefDefaultArrow() const;
    bool prefOrthoPyramid() const;

    TechDraw::DrawViewBalloon* getBalloonFeat()
    {
        return dvBalloon;
    }

    // balloons handle their own dragging
    void dragFinished() override { };

public Q_SLOTS:
    void balloonLabelDragged(bool ctrl);
    void balloonLabelDragFinished();
    void select(bool state);
    void hover(bool state);
    void updateBalloon(bool obtuse = false);

protected:
    void draw() override;
    void drawBalloon(bool originDrag = false);
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    virtual void setSvgPens();
    virtual void setPens();
    QString getPrecision();
    void parentViewMousePressed(QGIView* view, QPointF pos);
    TechDraw::DrawView* getSourceView() const;
    Base::Vector3d arrowPosInDrag();
    void getBalloonPoints(TechDraw::DrawViewBalloon* balloon,
                          TechDraw::DrawView* refObj,
                          bool isDragging,
                          Base::Vector3d& labelPos,
                          Base::Vector3d& arrowPos);


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
    Base::Vector3d m_saveOriginOffset;
    Base::Vector3d m_saveOrigin;
    Base::Vector3d m_savePosition;
};

}// namespace TechDrawGui