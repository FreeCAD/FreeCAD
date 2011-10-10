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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <Mod/Mesh/App/MeshFeature.h>
#include "PropertyEditorMesh.h"

using namespace MeshGui;
using MeshCore::MeshKernel;


TYPESYSTEM_SOURCE(MeshGui::PropertyMeshKernelItem, Gui::PropertyEditor::PropertyItem);

PropertyMeshKernelItem::PropertyMeshKernelItem()
{
    m_p = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_p->setParent(this);
    m_p->setPropertyName(QLatin1String("Points"));
    m_p->setReadOnly(true);
    this->appendChild(m_p);
    m_e = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_e->setParent(this);
    m_e->setPropertyName(QLatin1String("Edges"));
    m_e->setReadOnly(true);
    this->appendChild(m_e);
    m_f = static_cast<Gui::PropertyEditor::PropertyIntegerItem*>
        (Gui::PropertyEditor::PropertyIntegerItem::create());
    m_f->setParent(this);
    m_f->setPropertyName(QLatin1String("Faces"));
    m_f->setReadOnly(true);
    this->appendChild(m_f);
}

QVariant PropertyMeshKernelItem::value(const App::Property*) const
{
    int ctP = 0;
    int ctE = 0;
    int ctF = 0;

    std::vector<App::Property*> props = getPropertyData();
    for ( std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt ) {
        Mesh::PropertyMeshKernel* pPropMesh = (Mesh::PropertyMeshKernel*)(*pt);
        const MeshKernel& rMesh = pPropMesh->getValue().getKernel();
        ctP += (int)rMesh.CountPoints();
        ctE += (int)rMesh.CountEdges();
        ctF += (int)rMesh.CountFacets();
    }

    QString  str = QObject::tr("[Points: %1, Edges: %2 Faces: %3]").arg(ctP).arg(ctE).arg(ctF);
    return QVariant(str);
}

QVariant PropertyMeshKernelItem::toolTip(const App::Property* prop) const
{
    return value(prop);
}

void PropertyMeshKernelItem::setValue(const QVariant& value)
{
}

QWidget* PropertyMeshKernelItem::createEditor(QWidget* parent, const QObject* receiver, const char* method) const
{
    return 0;
}

void PropertyMeshKernelItem::setEditorData(QWidget *editor, const QVariant& data) const
{
}

QVariant PropertyMeshKernelItem::editorData(QWidget *editor) const
{
    return QVariant();
}

int PropertyMeshKernelItem::countPoints() const
{
    int ctP = 0;
    std::vector<App::Property*> props = getPropertyData();
    for ( std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt ) {
        Mesh::PropertyMeshKernel* pPropMesh = (Mesh::PropertyMeshKernel*)(*pt);
        const MeshKernel& rMesh = pPropMesh->getValue().getKernel();
        ctP += (int)rMesh.CountPoints();
    }

    return ctP;
}

int PropertyMeshKernelItem::countEdges() const
{
    int ctE = 0;
    std::vector<App::Property*> props = getPropertyData();
    for ( std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt ) {
        Mesh::PropertyMeshKernel* pPropMesh = (Mesh::PropertyMeshKernel*)(*pt);
        const MeshKernel& rMesh = pPropMesh->getValue().getKernel();
        ctE += (int)rMesh.CountEdges();
    }

    return ctE;
}

int PropertyMeshKernelItem::countFaces() const
{
    int ctF = 0;
    std::vector<App::Property*> props = getPropertyData();
    for ( std::vector<App::Property*>::const_iterator pt = props.begin(); pt != props.end(); ++pt ) {
        Mesh::PropertyMeshKernel* pPropMesh = (Mesh::PropertyMeshKernel*)(*pt);
        const MeshKernel& rMesh = pPropMesh->getValue().getKernel();
        ctF += (int)rMesh.CountFacets();
    }

    return ctF;
}

#include "moc_PropertyEditorMesh.cpp"
