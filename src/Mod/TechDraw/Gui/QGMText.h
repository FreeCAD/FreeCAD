/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

//QGMText.h
//a movable, editable text item

#ifndef TECHDRAWGUI_MOVABLETEXT_H
#define TECHDRAWGUI_MOVABLETEXT_H

#include "QGCustomText.h"
#include <QObject>


QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

//
class TechDrawGuiExport QGMText : public QGCustomText
{
Q_OBJECT

public:
    explicit QGMText(void);
    ~QGMText() {}

    enum {Type = QGraphicsItem::UserType + 300};
    int type() const override { return Type;}
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = nullptr ) override;

    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void showBox(bool b) { m_showBox = b; }
    virtual bool showBox(void) { return m_showBox; }

    virtual void setPrettyNormal() override;
    virtual void setPrettyPre() override;
    virtual void setPrettySel() override;

Q_SIGNALS:
    void dragging();
    void hover(bool state);
    void selected(bool state);
    void dragFinished();

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    bool m_showBox;
    std::string  m_prettyState;
    QPointF m_lastClick;

};

}

#endif // TECHDRAWGUI_MOVABLETEXT_H
