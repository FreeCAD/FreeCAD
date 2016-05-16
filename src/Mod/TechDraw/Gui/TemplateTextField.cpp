/***************************************************************************
 *   Copyright (c) Ian Rees                    (ian.rees@gmail.com) 2015   *
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
#include<QInputDialog>
#include<QLineEdit>
#endif // #ifndef _PreCmp_

#include "TemplateTextField.h"
#include "DlgTemplateField.h"

//#include<QDebug>

using namespace TechDrawGui;

TemplateTextField::TemplateTextField(QGraphicsItem*parent,
                                     TechDraw::DrawTemplate *myTmplte,
                                     const std::string &myFieldName)
    : QGraphicsRectItem(parent), tmplte(myTmplte), fieldNameStr(myFieldName)
{
}


TemplateTextField::~TemplateTextField()
{
}

void TemplateTextField::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    DlgTemplateField* ui = new DlgTemplateField(nullptr);
    ui->setFieldName(fieldNameStr);
    ui->setFieldContent(tmplte->EditableTexts[fieldNameStr]);
    int uiCode = ui->exec();
    std::string newContent = "";
    if(uiCode == QDialog::Accepted) {
       std::string newContent = ui->getFieldContent();
       tmplte->EditableTexts.setValue(fieldNameStr, newContent);
    }
    ui->deleteLater();
}
