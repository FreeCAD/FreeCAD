/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAWGUI_QGIHIGHLIGHT_H
#define TECHDRAWGUI_QGIHIGHLIGHT_H

#include <QColor>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QPointF>

#include "QGIArrow.h"
#include "QGCustomText.h"
#include "QGCustomRect.h"
#include "QGIDecoration.h"


namespace TechDrawGui
{

class TechDrawGuiExport QGIHighlight : public QGIDecoration
{
public:
    explicit QGIHighlight();
    ~QGIHighlight();

    enum {Type = QGraphicsItem::UserType + 176};
    int type() const override { return Type;}

    virtual void paint(QPainter * painter,
                       const QStyleOptionGraphicsItem * option, 
                       QWidget * widget = nullptr ) override;

    void setBounds(double x1,double y1,double x2,double y2);
    void setReference(const char* sym);
    void setFont(QFont f, double fsize);
    virtual void draw() override;
    void setInteractive(bool state);

protected:
/*    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;*/
/*    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;*/
/*    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;*/
    QColor getHighlightColor();
    Qt::PenStyle getHighlightStyle();
    void makeHighlight();
    void makeReference();
    void setTools();
    int getHoleStyle(void);

/*    bool m_dragging;*/

private:
    QString            m_refText;
    QGraphicsEllipseItem* m_circle;
    QGCustomRect*      m_rect;
    QGCustomText*      m_reference;
    std::string        m_refFontName;
    QFont              m_refFont;
    double             m_refSize;
    QPointF            m_start;
    QPointF            m_end;
};

}

#endif // TECHDRAWGUI_QGIHIGHLIGHT_H
