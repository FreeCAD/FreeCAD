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

#ifndef TECHDRAWGUI_QGISECTIONLINE_H
#define TECHDRAWGUI_QGISECTIONLINE_H

#include <QFont>
#include <QPointF>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QColor>

#include <Base/Vector3D.h>

#include "QGIArrow.h"
#include "QGCustomText.h"
#include "QGIDecoration.h"

namespace TechDrawGui
{

class TechDrawGuiExport QGISectionLine : public QGIDecoration
{
public:
    explicit QGISectionLine();
    ~QGISectionLine() {}

    enum {Type = QGraphicsItem::UserType + 172};
    int type() const { return Type;}

    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

    void setBounds(double x1,double y1,double x2,double y2);
    void setSymbol(char* sym);
    void setDirection(double xDir,double yDir);
    void setDirection(Base::Vector3d dir);
    void setFont(QFont f, double fsize);
    virtual void draw();

protected:
    QColor getSectionColor();
    Qt::PenStyle getSectionStyle();
    void makeLine();
    void makeArrows();
    void makeArrowsTrad();
    void makeArrowsISO();
    void makeSymbols();
    void makeSymbolsTrad();
    void makeSymbolsISO();
    void setTools();
    int  getPrefSectionFormat();

private:
    char* m_symbol;
    QGraphicsPathItem* m_line;           //primpath?
    QGIArrow*          m_arrow1;
    QGIArrow*          m_arrow2;
    QGCustomText*      m_symbol1;
    QGCustomText*      m_symbol2;
    QPointF            m_start;         //start of section line
    QPointF            m_end;           //end of section line
    Base::Vector3d     m_arrowDir;
    std::string        m_symFontName;
    QFont              m_symFont;
    double             m_symSize;
    double             m_arrowSize;
    //QColor             m_color;
    double             m_extLen;
//    int                m_sectionFormat;     //0 = ASME, 1 = ISO
};

}

#endif // TECHDRAWGUI_QGISECTIONLINE_H
