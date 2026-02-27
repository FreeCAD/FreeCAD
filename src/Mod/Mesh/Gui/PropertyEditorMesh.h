// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Gui/propertyeditor/PropertyItem.h>
#include <Mod/Mesh/MeshGlobal.h>

namespace MeshGui
{

using FrameOption = Gui::PropertyEditor::FrameOption;

/**
 * Display data of a mesh kernel.
 * \author Werner Mayer
 */
class MeshGuiExport PropertyMeshKernelItem: public Gui::PropertyEditor::PropertyItem
{
    Q_OBJECT
    Q_PROPERTY(int Points READ countPoints CONSTANT)
    Q_PROPERTY(int Edges READ countEdges CONSTANT)
    Q_PROPERTY(int Faces READ countFaces CONSTANT)
    PROPERTYITEM_HEADER

    // clang-format off
    QWidget* createEditor(QWidget* parent,
                          const std::function<void()>& method,
                          FrameOption frameOption) const override;
    // clang-format on
    void setEditorData(QWidget* editor, const QVariant& data) const override;
    QVariant editorData(QWidget* editor) const override;

    int countPoints() const;
    int countEdges() const;
    int countFaces() const;

protected:
    QVariant toolTip(const App::Property*) const override;
    QVariant value(const App::Property*) const override;
    void setValue(const QVariant&) override;

protected:
    PropertyMeshKernelItem();
    void initialize() override;

private:
    Gui::PropertyEditor::PropertyIntegerItem* m_p;
    Gui::PropertyEditor::PropertyIntegerItem* m_e;
    Gui::PropertyEditor::PropertyIntegerItem* m_f;
};

}  // namespace MeshGui
