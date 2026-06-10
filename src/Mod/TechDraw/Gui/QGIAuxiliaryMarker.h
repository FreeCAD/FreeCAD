/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                               *
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
#include <QGraphicsPathItem>
#include <QPen>
#include <QPointF>
#include <QString>

#include <Base/Vector3D.h>

#include "QGIDecoration.h"
#include "QGIUserTypes.h"

namespace TechDrawGui
{

class QGIArrow;
class QGCustomText;

class TechDrawGuiExport QGIAuxiliaryMarker: public QGIDecoration
{
public:
    explicit QGIAuxiliaryMarker();
    ~QGIAuxiliaryMarker() override = default;

    enum
    {
        Type = UserType::QGIAuxiliaryMarker
    };
    int type() const override
    {
        return Type;
    }

    void setEnds(const Base::Vector3d& start, const Base::Vector3d& end);
    void setDirection(const Base::Vector3d& direction);
    void setLabel(const QString& label);
    void setFont(const QFont& font);
    void setArrowSize(double arrowSize);
    void setLinePen(const QPen& pen);
    void setMarkerColor(const QColor& color);
    void draw() override;

private:
    void drawReferenceLine();
    void drawArrow(QGIArrow* arrow, const QPointF& position);
    void drawLabel(QGCustomText* label, const QPointF& position);
    QPointF labelPosition(const QPointF& reference) const;

    QGraphicsPathItem* m_line;
    QGIArrow* m_arrow1;
    QGIArrow* m_arrow2;
    QGCustomText* m_label1;
    QGCustomText* m_label2;
    QPointF m_start;
    QPointF m_end;
    Base::Vector3d m_direction;
    QString m_labelText;
    QFont m_font;
    double m_arrowSize;
    QColor m_color;
};

}  // namespace TechDrawGui
