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

#include <QFont>
#include <QPointF>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QPainterPath>
#include <QColor>

#include <Base/Vector3D.h>

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
    ~QGIHighlight() {}

    enum {Type = QGraphicsItem::UserType + 172};
    int type() const { return Type;}

    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

    void setBounds(double x1,double y1,double x2,double y2);
    void setReference(char* sym);
    void setFont(QFont f, double fsize);
    virtual void draw();

protected:
    QColor getHighlightColor();
    Qt::PenStyle getHighlightStyle();
    void makeHighlight();
    void makeReference();
    void setTools();
    int getHoleStyle(void);


private:
    char* m_refText;
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
