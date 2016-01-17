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
    //TODO: Add a command to change template text, and call it from here
    bool ok;
    QString curStr = QString::fromUtf8(tmplte->EditableTexts[fieldNameStr].c_str());
    QString newStr = QInputDialog::getText(NULL, QObject::tr("Change template text"),
                                           QObject::tr("Enter a new value for ") +
                                               QString::fromUtf8(fieldNameStr.c_str()),
                                           QLineEdit::Normal, curStr, &ok);
    if (ok && !newStr.isEmpty()) {
        tmplte->EditableTexts.setValue(fieldNameStr, newStr.toUtf8().constData());
    }
}

