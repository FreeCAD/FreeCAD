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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <SMESH_Mesh.hxx>
#endif

#include "PropertyFemMeshItem.h"
#include <Mod/Fem/App/FemMeshProperty.h>


using namespace FemGui;


PROPERTYITEM_SOURCE(FemGui::PropertyFemMeshItem)

PropertyFemMeshItem::PropertyFemMeshItem()
{
    m_n = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_n->setParent(this);
    m_n->setPropertyName(QLatin1String("Nodes"));
    this->appendChild(m_n);
    m_e = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_e->setParent(this);
    m_e->setPropertyName(QLatin1String("Edges"));
    this->appendChild(m_e);
    m_f = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_f->setParent(this);
    m_f->setPropertyName(QLatin1String("Faces"));
    this->appendChild(m_f);
    m_p = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_p->setParent(this);
    m_p->setPropertyName(QLatin1String("Polygons"));
    this->appendChild(m_p);
    m_v = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_v->setParent(this);
    m_v->setPropertyName(QLatin1String("Volumes"));
    this->appendChild(m_v);
    m_h = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_h->setParent(this);
    m_h->setPropertyName(QLatin1String("Polyhedrons"));
    this->appendChild(m_h);
    m_g = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_g->setParent(this);
    m_g->setPropertyName(QLatin1String("Groups"));
    this->appendChild(m_g);
}

void PropertyFemMeshItem::initialize()
{
    this->setReadOnly(true);
}

QVariant PropertyFemMeshItem::value(const App::Property*) const
{
    int ctN = 0;
    int ctE = 0;
    int ctF = 0;
    int ctP = 0;
    int ctV = 0;
    int ctH = 0;
    int ctG = 0;

    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctN += mesh->NbNodes();
        ctE += mesh->NbEdges();
        ctF += mesh->NbFaces();
        ctP += mesh->NbPolygons();
        ctV += mesh->NbVolumes();
        ctH += mesh->NbPolyhedrons();
        ctG += mesh->NbGroup();
    }

    QString  str = QObject::tr("[Nodes: %1, Edges: %2, Faces: %3, Polygons: %4, Volumes: %5, Polyhedrons: %6, Groups: %7]")
        .arg(ctN).arg(ctE).arg(ctF).arg(ctP).arg(ctV).arg(ctH).arg(ctG);
    return QVariant(str);
}

QVariant PropertyFemMeshItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

void PropertyFemMeshItem::setValue(const QVariant& value)
{
    Q_UNUSED(value);
}

QWidget* PropertyFemMeshItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    Q_UNUSED(parent);
    Q_UNUSED(receiver);
    Q_UNUSED(method);
    return 0;
}

void PropertyFemMeshItem::setEditorData(QWidget *editor, const QVariant& data) const
{
    Q_UNUSED(editor);
    Q_UNUSED(data);
}

QVariant PropertyFemMeshItem::editorData(QWidget *editor) const
{
    Q_UNUSED(editor);
    return QVariant();
}

int PropertyFemMeshItem::countNodes() const
{
    int ctN = 0;
    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctN += mesh->NbNodes();
    }

    return ctN;
}

int PropertyFemMeshItem::countEdges() const
{
    int ctE = 0;
    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctE += mesh->NbEdges();
    }

    return ctE;
}

int PropertyFemMeshItem::countFaces() const
{
    int ctF = 0;
    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctF += mesh->NbFaces();
    }

    return ctF;
}

int PropertyFemMeshItem::countPolygons() const
{
    int ctP = 0;
    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctP += mesh->NbPolygons();
    }

    return ctP;
}

int PropertyFemMeshItem::countVolumes() const
{
    int ctV = 0;
    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctV += mesh->NbVolumes();
    }

    return ctV;
}

int PropertyFemMeshItem::countPolyhedrons() const
{
    int ctH = 0;
    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctH += mesh->NbPolyhedrons();
    }

    return ctH;
}

int PropertyFemMeshItem::countGroups() const
{
    int ctG = 0;
    const std::vector<App::Property*>& props = getPropertyData();
    for (std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt) {
        Fem::PropertyFemMesh* prop = static_cast<Fem::PropertyFemMesh*>(*pt);
        SMESH_Mesh* mesh = const_cast<SMESH_Mesh*>(prop->getValue().getSMesh());
        ctG += mesh->NbGroup();
    }

    return ctG;
}

#include "moc_PropertyFemMeshItem.cpp"
