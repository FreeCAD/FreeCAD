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

#ifndef DRAWINGGUI_TEMPLATETEXTFIELD_H
#define DRAWINGGUI_TEMPLATETEXTFIELD_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsRectItem>

namespace TechDraw {
class DrawTemplate;
}

namespace TechDrawGui
{
    /// QGraphicsRectItem-derived class for the text fields in title blocks
    /*!
     * Makes an area on the drawing that's clickable, so appropriate
     * Properties of the template can be modified.
     */
class TechDrawGuiExport TemplateTextField : public QGraphicsRectItem
{
    public:
        TemplateTextField(QGraphicsItem *parent,
                          TechDraw::DrawTemplate *myTmplte,
                          const std::string &myFieldName);

        ~TemplateTextField() override = default;

        enum {Type = QGraphicsItem::UserType + 160};
        int type() const override { return Type;}

        /// Returns the field name that this TemplateTextField represents
        std::string fieldName() const { return fieldNameStr; }

    protected:
        TechDraw::DrawTemplate *tmplte;
        std::string fieldNameStr;

        /// Need this to properly handle mouse release
        void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

        /// Trigger the dialog for editing template text
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
};
}   // namespace TechDrawGui

#endif // #ifndef DRAWINGGUI_TEMPLATETEXTFIELD_H
