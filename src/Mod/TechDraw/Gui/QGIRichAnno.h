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

#ifndef TECHDRAWGUI_QGIRICHANNO_H
#define TECHDRAWGUI_QGIRICHANNO_H

#include <QObject>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QPainterPath>
#include <QColor>
#include <QFont>
#include <QPointF>

#include <Base/Vector3D.h>
#include "QGILeaderLine.h"

namespace TechDraw {
class DrawRichAnno;
class DrawLeaderLine;
}

namespace TechDrawGui
{
class QGIPrimPath;
class QGIArrow;
class QGEPath;
class QGMText;


//*******************************************************************

class TechDrawGuiExport QGIRichAnno : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 233};

    explicit QGIRichAnno(QGraphicsItem* myParent = nullptr,
                           TechDraw::DrawRichAnno* lead = nullptr);
    ~QGIRichAnno() = default;

    int type() const override { return Type;}
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;
    virtual QRectF boundingRect() const override;
    virtual QPainterPath shape(void) const override;

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;

    void setTextItem(void);

    virtual TechDraw::DrawRichAnno* getFeature(void);

public Q_SLOTS:
    void textDragging(void);
    void textDragFinished(void);
    void hover(bool state);
    void select(bool state);

protected:
    virtual void draw() override;
    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;

protected:
    QGMText* m_text;
    bool m_hasHover;

};

}

#endif // TECHDRAWGUI_QGIRICHANNO_H
