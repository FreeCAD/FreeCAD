/***************************************************************************
 *   Copyright (c) 2017 Peter Lama <peterldev94@gmail.com>                 *
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


#ifndef PART_PropertyEnumAttacherItem_H
#define PART_PropertyEnumAttacherItem_H

#include <Gui/propertyeditor/PropertyItem.h>
#include "TaskAttacher.h"

namespace PartGui
{

/**
* Custom editor item for PropertyEnumeration to open Attacher task
*/
class PartGuiExport PropertyEnumAttacherItem: public Gui::PropertyEditor::PropertyEnumItem
{
    Q_OBJECT

public:
    PROPERTYITEM_HEADER

    QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected Q_SLOTS:
    void openTask();

protected:
    PropertyEnumAttacherItem();
};

} // namespace PartGui

#endif // PART_PropertyEnumAttacherItem_H
