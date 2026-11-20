// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <QObject>
#include <vector>

#include <Gui/propertyeditor/PropertyItem.h>


namespace SketcherGui
{

using FrameOption = Gui::PropertyEditor::FrameOption;

class PropertyConstraintListItem: public Gui::PropertyEditor::PropertyItem
{
    Q_OBJECT
    PROPERTYITEM_HEADER

    ~PropertyConstraintListItem() override;
    void assignProperty(const App::Property* prop) override;
    QWidget* createEditor(
        QWidget* parent,
        const std::function<void()>& method,
        FrameOption frameOption
    ) const override;
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

protected:
    QString toString(const QVariant&) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;
    bool event(QEvent* ev) override;

    void initialize() override;

protected:
    PropertyConstraintListItem();
    bool blockEvent;
    bool onlyUnnamed;
};

}  // namespace SketcherGui
