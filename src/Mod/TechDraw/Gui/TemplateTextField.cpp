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


  #include <QGraphicsSceneMouseEvent>
  #include <QInputDialog>
  #include <QLineEdit>
  #include <QTextDocument>

#include <Base/Console.h>
#include <Gui/MainWindow.h>

#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>

#include "DlgTemplateField.h"
#include "TemplateTextField.h"

using namespace TechDrawGui;
using namespace TechDraw;

TemplateTextField::TemplateTextField(QGraphicsItem *parent,
                                     TechDraw::DrawTemplate *myTmplte,
                                     const std::string &myFieldName)
    : QGraphicsItemGroup(parent),
      tmplte(myTmplte),
      fieldName(myFieldName),
      m_rect(new QGraphicsRectItem()),
      m_line(new QGraphicsPathItem())
{
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setAcceptHoverEvents(true);
    setFiltersChildEvents(true);

    setToolTip(QObject::tr("Click to update text"));

    addToGroup(m_rect);
    QPen rectPen(Qt::transparent);
    QBrush rectBrush(Qt::NoBrush);
    m_rect->setPen(rectPen);
    m_rect->setBrush(rectBrush);
    m_rect->setAcceptHoverEvents(true);

    m_line->hide();
    addToGroup(m_line);
 }

void TemplateTextField::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if ( tmplte && m_rect->rect().contains(event->pos()) ) {
        event->accept();
    } else {
        QGraphicsItemGroup::mousePressEvent(event);
    }
}

void TemplateTextField::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if ( tmplte && m_rect->rect().contains(event->pos()) ) {
        event->accept();

        DlgTemplateField ui(Gui::getMainWindow());
        ui.setTemplate(tmplte);

        ui.setFieldName(QString::fromStdString(fieldName));
        ui.setFieldContent(QString::fromStdString(tmplte->EditableTexts.getValue(fieldName)));
        ui.setAutofillContent(QString::fromStdString(tmplte->getAutofillValue(autofillId)));

        std::ostringstream ss;
        ss << "Edit field " << fieldName << " in " << tmplte->Label.getValue();
        App::GetApplication().setActiveTransaction(ss.str().c_str());

        int result = ui.exec();
        if (result == QDialog::Accepted) {
            tmplte->EditableTexts.setValue(fieldName, ui.getFieldContent().toStdString());
        }

        App::GetApplication().closeActiveTransaction(result != QDialog::Accepted);
    }
    else {
        QGraphicsItemGroup::mouseReleaseEvent(event);
    }
}

void TemplateTextField::setRectangle(QRectF rect)
{
    m_rect->setRect(rect);
}

void TemplateTextField::setLine(QPointF from, QPointF to)
{
    QPainterPath path(from);
    path.lineTo(to);
    m_line->setPath(path);
}

void TemplateTextField::setLineColor(QColor color)
{
    QPen pen(color);
    constexpr int LineWidth{5};
    pen.setWidth(LineWidth);
    m_line->setPen(pen);
}

void TemplateTextField::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    showLine();
    QGraphicsItemGroup::hoverEnterEvent(event);
}

void TemplateTextField::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    hideLine();
    QGraphicsItemGroup::hoverLeaveEvent(event);
}


