/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef FEMGUI_PROPERTY_FEMMESH_ITEM_H
#define FEMGUI_PROPERTY_FEMMESH_ITEM_H

#include <Gui/propertyeditor/PropertyItem.h>

namespace FemGui {

/**
 * Display data of an FEM mesh.
 * \author Werner Mayer
 */
class PropertyFemMeshItem : public Gui::PropertyEditor::PropertyItem
{
    Q_OBJECT
    Q_PROPERTY(int Nodes READ countNodes)
    Q_PROPERTY(int Edges READ countEdges)
    Q_PROPERTY(int Faces READ countFaces)
    Q_PROPERTY(int Polygons READ countPolygons)
    Q_PROPERTY(int Volumes READ countVolumes)
    Q_PROPERTY(int Polyhedrons READ countPolyhedrons)
    TYPESYSTEM_HEADER();

    virtual QWidget* createEditor(QWidget* parent, const QObject* receiver, const char* method) const;
    virtual void setEditorData(QWidget *editor, const QVariant& data) const;
    virtual QVariant editorData(QWidget *editor) const;

    int countNodes() const;
    int countEdges() const;
    int countFaces() const;
    int countPolygons() const;
    int countVolumes() const;
    int countPolyhedrons() const;

protected:
    virtual QVariant toolTip(const App::Property*) const;
    virtual QVariant value(const App::Property*) const;
    virtual void setValue(const QVariant&);

protected:
    PropertyFemMeshItem();
    void initialize();

private:
    Gui::PropertyEditor::PropertyIntegerItem* m_n;
    Gui::PropertyEditor::PropertyIntegerItem* m_e;
    Gui::PropertyEditor::PropertyIntegerItem* m_f;
    Gui::PropertyEditor::PropertyIntegerItem* m_p;
    Gui::PropertyEditor::PropertyIntegerItem* m_v;
    Gui::PropertyEditor::PropertyIntegerItem* m_h;
};

} // namespace FemGui


#endif // FEMGUI_PROPERTY_FEMMESH_ITEM_H

