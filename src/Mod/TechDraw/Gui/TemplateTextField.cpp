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
#include <Mod/TechDraw/App/DrawTemplate.h>

#include "DlgTemplateField.h"
#include "TemplateTextField.h"

using namespace TechDrawGui;

TemplateTextField::TemplateTextField(QGraphicsItem *parent,
                                     TechDraw::DrawTemplate *myTmplte,
                                     const std::string &myFieldName)
    : QGraphicsRectItem(parent),
      tmplte(myTmplte),
      fieldNameStr(myFieldName)
{
    setToolTip(QObject::tr("Click to update text"));
 }

void TemplateTextField::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if ( tmplte && rect().contains(event->pos()) ) {
        event->accept();
    } else {
        QGraphicsRectItem::mousePressEvent(event);
    }
}

void TemplateTextField::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if ( tmplte && rect().contains(event->pos()) ) {
        event->accept();

        DlgTemplateField ui;

        ui.setFieldName(fieldNameStr);
        ui.setFieldContent(tmplte->EditableTexts[fieldNameStr]);

        if (ui.exec() == QDialog::Accepted) {
        //WF: why is this escaped?
        //    "<" is converted elsewhere and no other characters cause problems.
        //    escaping causes "&" to appear as "&amp;" etc
//            QString qsClean = ui.getFieldContent().toHtmlEscaped();
            QString qsClean = ui.getFieldContent();
            std::string utf8Content = qsClean.toUtf8().constData();
            tmplte->EditableTexts.setValue(fieldNameStr, utf8Content);
        }

    } else {
        QGraphicsRectItem::mouseReleaseEvent(event);
    }
}

