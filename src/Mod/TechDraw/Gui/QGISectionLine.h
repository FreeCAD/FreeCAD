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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QFont>
#include <QPainterPath>
#include <QPointF>

#include <Base/Vector3D.h>
#include <Mod/TechDraw/App/DrawViewSection.h>

#include "QGCustomText.h"
#include "QGIDecoration.h"


namespace TechDrawGui
{

class QGIArrow;
class QGCustomText;

class TechDrawGuiExport QGISectionLine : public QGIDecoration
{
public:
    explicit QGISectionLine();
    ~QGISectionLine() override = default;

    enum {Type = QGraphicsItem::UserType + 172};
    int type() const override { return Type;}

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

    void setEnds(Base::Vector3d l1, Base::Vector3d l2);
    void setBounds(double x1, double y1, double x2, double y2);
    void setPath(QPainterPath& path);
    void setSymbol(char* sym);
    void setDirection(double xDir, double yDir);
    void setDirection(Base::Vector3d dir);
    void setArrowDirections(Base::Vector3d dir1, Base::Vector3d dir2);
    void setFont(QFont f, double fsize);
    void setSectionStyle(int style);
    void setSectionColor(QColor c);
    void setPathMode(bool mode) { m_pathMode = mode; }
    bool pathMode() { return m_pathMode; }
    void setChangePoints(TechDraw::ChangePointVector changePoints);
    void clearChangePoints();
    void draw() override;

protected:
    QColor getSectionColor();
    Qt::PenStyle getSectionStyle();
    void makeSectionLine();
    void makeExtensionLine();
    void makeArrows();
    void makeArrowsTrad();
    void makeArrowsISO();
    void makeSymbols();
    void makeSymbolsTrad();
    void makeSymbolsISO();
    void makeChangePointMarks();
    void setTools();
    int  getPrefSectionStandard();
    void extensionEndsISO();
    void extensionEndsTrad();
    double getArrowRotation(Base::Vector3d arrowDir);
    QPointF getArrowPosition(Base::Vector3d arrowDir, QPointF refPoint);
    void clearChangePointMarks();

    static QPointF normalizeQPointF(QPointF inPoint);

private:
    char* m_symbol;
    QGraphicsPathItem* m_line;
    QGraphicsPathItem* m_extend;
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
    double             m_extLen;
    Base::Vector3d     m_l1;            //end of main section line
    Base::Vector3d     m_l2;            //end of main section line
    QPointF            m_beginExt1;     //start of extension line 1
    QPointF            m_endExt1;       //end of extension line 1
    QPointF            m_beginExt2;     //start of extension line 2
    QPointF            m_endExt2;       //end of extension line 1
    bool               m_pathMode;      //use external path for line
    int                m_arrowMode;     //0 = 1 direction for both arrows, 1 = direction for each arrow
    Base::Vector3d     m_arrowDir1;
    Base::Vector3d     m_arrowDir2;
    QPointF            m_arrowPos1;
    QPointF            m_arrowPos2;
    std::vector<QGraphicsPathItem*> m_changePointMarks;
    TechDraw::ChangePointVector m_changePointData;
};

}

#endif // TECHDRAWGUI_QGISECTIONLINE_H
