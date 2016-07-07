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

#ifndef DRAWINGGUI_QGRAPHICSITEMFACE_H
#define DRAWINGGUI_QGRAPHICSITEMFACE_H

#include <Qt>
#include <QGraphicsItem>
#include <QSvgRenderer>
#include <QByteArray>

#include "QGIPrimPath.h"
#include "QGCustomSvg.h"
#include "QGCustomRect.h"

namespace TechDrawGeometry {
class BaseGeom;
}

namespace TechDrawGui
{

    const double SVGSIZEW = 64.0;                     //width and height of standard FC SVG pattern
    const double SVGSIZEH = 64.0;
    const std::string  SVGCOLPREFIX = "stroke:";
    const std::string  SVGCOLDEFAULT = "#000000";

class QGIFace : public QGIPrimPath
{
public:
    explicit QGIFace(int index = -1);
    ~QGIFace();

    enum {Type = QGraphicsItem::UserType + 104};
    int type() const { return Type;}
    QRectF boundingRect() const;
    QPainterPath shape() const;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

public:
    int getProjIndex() const { return projIndex; }

    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();
    void setFill(QColor c, Qt::BrushStyle s);
    void setFill(QBrush b);
    void setHatch(std::string fileSpec);
    void resetFill(void);
    void setPath(const QPainterPath & path);
    void buildHatch(void);
    void setHatchColor(std::string c);

protected:
    bool load(QByteArray *svgBytes);

protected:
    int projIndex;                              //index of face in Projection. -1 for SectionFace.
    QGCustomRect *m_rect;
    QGCustomSvg *m_svg;
    QByteArray m_svgXML;
    std::string m_svgCol;

private:
    QBrush m_brush;
    Qt::BrushStyle m_fillStyle;                 //current fill style
    QColor m_fillColor;                         //current fill color

    QColor m_colDefFill;                        //"no color" default normal fill color
    QColor m_colNormalFill;                     //current Normal fill color
    Qt::BrushStyle m_styleDef;                  //default Normal fill style
    Qt::BrushStyle m_styleNormal;               //current Normal fill style
    Qt::BrushStyle m_styleSelect;               //Select/preSelect fill style
};

}
#endif // DRAWINGGUI_QGRAPHICSITEMFACE_H
