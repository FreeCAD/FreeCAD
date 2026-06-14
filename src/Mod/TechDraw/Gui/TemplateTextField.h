/***************************************************************************
 *   Copyright (c) 2015 Ian Rees <ian.rees@gmail.com>                      *
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

#include <QGraphicsItemGroup>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>

#include "QGIUserTypes.h"

namespace TechDraw {
class DrawTemplate;
}

namespace TechDrawGui
{
    /// QGraphicsItemGroupm-derived class for the text fields in title blocks
    /*!
     * Makes an area on the drawing that's clickable, so appropriate
     * Properties of the template can be modified.
     */
class TechDrawGuiExport TemplateTextField : public QGraphicsItemGroup
{
    public:
        TemplateTextField(QGraphicsItem *parent,
                          TechDraw::DrawTemplate *myTmplte,
                          const std::string &myFieldName);

        ~TemplateTextField() override = default;

        enum {Type = UserType::TemplateTextField};
        int type() const override { return Type;}

        /// Returns the field name that this TemplateTextField represents
        std::string fieldName() const { return fieldNameStr; }

        void setAutofill(const QString& autofillString);
        void setRectangle(QRectF rect);
        void setLine(QPointF from, QPointF to);
        void setLineColor(QColor color);
        void hideLine() { m_line->hide(); }
        void showLine() { m_line->show(); }

    protected:
        /// Need this to properly handle mouse release
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

        /// Trigger the dialog for editing template text
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

        void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
        void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    private:
        TechDraw::DrawTemplate *tmplte;
        std::string fieldNameStr;
        QString m_autofillString;

        QGraphicsRectItem* m_rect;
        QGraphicsPathItem* m_line;
};
}   // namespace TechDrawGui