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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QPointF>

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

    void paint(QPainter * painter,
               const QStyleOptionGraphicsItem * option,
               QWidget * widget = nullptr ) override;

    void setBounds(double x1, double y1, double x2, double y2);
    void setReference(const char* sym);
    void setFont(QFont f, double fsize);
    virtual void draw() override;
    void setInteractive(bool state);
    void setFeatureName(std::string name) { m_featureName = name; }
    std::string getFeatureName() { return m_featureName; }
    void setReferenceAngle(double angle) { m_referenceAngle = angle; }

    void onDragFinished() override;

protected:
    QColor getHighlightColor();
    Qt::PenStyle getHighlightStyle();
    void makeHighlight();
    void makeReference();
    void setTools();
    int getHoleStyle(void);

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
    std::string        m_featureName;
    double             m_referenceAngle;
};

}

#endif // TECHDRAWGUI_QGIHIGHLIGHT_H
