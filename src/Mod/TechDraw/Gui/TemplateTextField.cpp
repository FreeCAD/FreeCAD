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

#include "PreCompiled.h"

#ifndef _PreComp_
  #include <QGraphicsSceneMouseEvent>
  #include <QInputDialog>
  #include <QLineEdit>
  #include <QTextDocument>
#endif // #ifndef _PreCmp_

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
      fieldNameStr(myFieldName)
{
    setToolTip(QObject::tr("Updates text"));
    m_rect = new QGraphicsRectItem();
    addToGroup(m_rect);
    QPen rectPen(Qt::transparent);
    QBrush rectBrush(Qt::NoBrush);
    m_rect->setPen(rectPen);
    m_rect->setBrush(rectBrush);

    m_line = new QGraphicsPathItem();
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

        ui.setFieldName(fieldNameStr);
        ui.setFieldContent(tmplte->EditableTexts[fieldNameStr]);

        auto qName = QString::fromStdString(fieldNameStr);
        auto svgTemplate = freecad_cast<DrawSVGTemplate*>(tmplte);
        if (svgTemplate) {
            // preset the autofill with the current value - something might have changed since this field was created
            m_autofillString = svgTemplate->getAutofillByEditableName(qName);
        }
        ui.setAutofillContent(m_autofillString.toStdString());

        if (ui.exec() == QDialog::Accepted) {
            QString qsClean = ui.getFieldContent();
            std::string utf8Content = qsClean.toUtf8().constData();
            if (ui.getAutofillState()) {
                if (svgTemplate) {
                    // unlikely, but something could have changed since we grabbed the autofill value
                    QString fieldName = QString::fromStdString(fieldNameStr);
                    QString autofillValue = svgTemplate->getAutofillByEditableName(fieldName);
                    if (!autofillValue.isEmpty()) {
                        utf8Content = autofillValue.toUtf8().constData();
                    }
                }
            }
            tmplte->EditableTexts.setValue(fieldNameStr, utf8Content);
        }

    } else {
        QGraphicsItemGroup::mouseReleaseEvent(event);
    }
}

//void setAutofill(std::string autofillString);
void TemplateTextField::setAutofill(QString autofillString)
{
    m_autofillString = autofillString;
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
    pen.setWidth(5);
    m_line->setPen(pen);
}
