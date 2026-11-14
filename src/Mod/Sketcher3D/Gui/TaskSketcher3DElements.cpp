// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <QAbstractItemView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

#include <BRep_Tool.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection/Selection.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Sketcher3D/App/GeomReferencePlane3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "TaskSketcher3DElements.h"
#include "TaskSketcher3DPanelHelpers.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;
using Sketcher3DGui::PanelHelpers::buildGeometryLabels;

TaskSketcher3DElements::TaskSketcher3DElements(ViewProviderSketch3D* view)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_CreateLine"), tr("Elements"), true, nullptr)
    , sketchView(view)
{
    auto* body = new QWidget(this);
    auto* root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 0, 0);

    elementsList = new QListWidget(body);
    elementsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    elementsList->setUniformItemSizes(true);
    elementsList->setAlternatingRowColors(true);
    elementsList->setMinimumHeight(100);
    connect(elementsList, &QListWidget::itemClicked, this, &TaskSketcher3DElements::onElementRowClicked);
    root->addWidget(elementsList);

    addWidget(body, true, false);

    connectionElementsChanged = sketchView->signalElementsChanged.connect([this]() { refresh(); });
    refresh();
}

TaskSketcher3DElements::~TaskSketcher3DElements()
{
    connectionElementsChanged.disconnect();
}

void TaskSketcher3DElements::refresh()
{
    elementsList->clear();

    auto* sketch = sketchView->getSketch3DObject();

    auto fmtVec = [](const Base::Vector3d& v) {
        return QStringLiteral("(%1, %2, %3)").arg(v.x, 0, 'f', 3).arg(v.y, 0, 'f', 3).arg(v.z, 0, 'f', 3);
    };
    auto addRow = [this](const QString& text, const QString& subname, const QIcon& icon) {
        auto* item = new QListWidgetItem(text, elementsList);
        item->setIcon(icon);
        item->setData(Qt::UserRole, subname);
    };

    auto& geos = sketch->Geometry.getValues();
    std::map<int, int> lineLabelForGeoId;
    std::map<int, int> pointLabelForGeoId;
    buildGeometryLabels(geos, pointLabelForGeoId, lineLabelForGeoId);

    auto appendFromShape = [&](const Part::TopoShape& src, const QString& prefix) {
        unsigned long edgeCount = src.countSubShapes(TopAbs_EDGE);
        for (unsigned long i = 1; i <= edgeCount; ++i) {
            TopoDS_Shape sub;
            try {
                sub = src.getSubShape(TopAbs_EDGE, static_cast<int>(i), true);
            }
            catch (const Standard_Failure&) {
                continue;
            }
            if (sub.IsNull()) {
                continue;
            }

            QString subname = prefix + QStringLiteral("Edge%1").arg(static_cast<qulonglong>(i));
            auto id = sketch->resolveSubName(subname.toStdString());
            if (!id.isValid() || id.GeoId < 0 || id.GeoId >= static_cast<int>(geos.size())) {
                continue;
            }
            auto* ls = dynamic_cast<Part::GeomLineSegment*>(geos[id.GeoId]);
            if (!ls) {
                continue;
            }
            addRow(
                tr("Line%1  length %2")
                    .arg(lineLabelForGeoId[id.GeoId])
                    .arg((ls->getEndPoint() - ls->getStartPoint()).Length(), 0, 'f', 3),
                subname,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine")
            );
        }

        unsigned long vertexCount = src.countSubShapes(TopAbs_VERTEX);
        for (unsigned long i = 1; i <= vertexCount; ++i) {
            TopoDS_Shape sub;
            try {
                sub = src.getSubShape(TopAbs_VERTEX, static_cast<int>(i), true);
            }
            catch (const Standard_Failure&) {
                continue;
            }
            if (sub.IsNull()) {
                continue;
            }

            QString subname = prefix + QStringLiteral("Vertex%1").arg(static_cast<qulonglong>(i));
            auto id = sketch->resolveSubName(subname.toStdString());
            if (!id.isValid()) {
                continue;
            }

            gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(sub));
            Base::Vector3d pos(p.X(), p.Y(), p.Z());

            QString text;
            if (id.Kind == Sketcher3D::GeoKind::Point) {
                text = tr("Point%1  %2").arg(pointLabelForGeoId[id.GeoId]).arg(fmtVec(pos));
            }
            else if (id.Kind == Sketcher3D::GeoKind::Line) {
                text = id.Pos == Sketcher3D::PointPos::start
                    ? tr("  Line%1.start  %2").arg(lineLabelForGeoId[id.GeoId]).arg(fmtVec(pos))
                    : tr("  Line%1.end  %2").arg(lineLabelForGeoId[id.GeoId]).arg(fmtVec(pos));
            }
            if (!text.isEmpty()) {
                addRow(text, subname, Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePoint"));
            }
        }
    };

    appendFromShape(sketch->Shape.getShape(), {});
    appendFromShape(
        sketch->ReferenceShape.getShape(),
        QString::fromStdString(Sketcher3D::Sketch3DObject::referencePrefix())
    );

    auto refPrefix = QString::fromStdString(Sketcher3D::Sketch3DObject::referencePrefix());
    for (int i = 0; i < static_cast<int>(geos.size()); ++i) {
        if (!geos[i] || !geos[i]->is<Sketcher3D::GeomReferencePlane3D>()) {
            continue;
        }
        addRow(
            tr("Plane%1").arg(i + 1),
            refPrefix + QStringLiteral("Face%1").arg(i + 1),
            Gui::BitmapFactory().iconFromTheme("Std_Plane")
        );
    }

    setHeaderText(tr("Elements (%1)").arg(elementsList->count()));
}

void TaskSketcher3DElements::onElementRowClicked(QListWidgetItem* item)
{
    auto* sketch = sketchView->getSketch3DObject();
    std::string sub = item->data(Qt::UserRole).toString().toStdString();

    Gui::Selection().clearSelection();
    Gui::Selection()
        .addSelection(sketch->getDocument()->getName(), sketch->getNameInDocument(), sub.c_str());
}

#include "moc_TaskSketcher3DElements.cpp"
