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

#include <map>

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include <BRep_Tool.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection/Selection.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Sketcher3D/App/Constraint3D.h>
#include <Mod/Sketcher3D/App/PropertyConstraint3DList.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "TaskSketcher3DTool.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;


TaskSketcher3DTool::TaskSketcher3DTool(ViewProviderSketch3D* view)
    : TaskBox(
          Gui::BitmapFactory().pixmap("Sketcher_Sketch"),
          view && view->getObject() ? QString::fromUtf8(view->getObject()->Label.getValue())
                                    : tr("Sketch3D Edit"),
          true,
          nullptr
      )
    , sketchView(view)
{
    auto* body = new QWidget(this);
    auto* root = new QVBoxLayout(body);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(6);

    hintLabel = new QLabel();
    hintLabel->setWordWrap(true);
    setHint(tr(
        "Pick a tool from the toolbar, then click in the 3D view. "
        "Tab cycles the active workplane (XY / YZ / ZX)."
    ));
    root->addWidget(hintLabel);

    elementsHeader = new QLabel();
    root->addWidget(elementsHeader);

    elementsList = new QListWidget();
    elementsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    elementsList->setUniformItemSizes(true);
    elementsList->setMinimumHeight(100);
    connect(
        elementsList,
        &QListWidget::itemClicked,
        this,
        &TaskSketcher3DTool::onElementRowClicked
    );
    root->addWidget(elementsList);

    constraintsHeader = new QLabel();
    root->addWidget(constraintsHeader);

    constraintsList = new QListWidget();
    constraintsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    constraintsList->setUniformItemSizes(true);
    constraintsList->setMinimumHeight(80);
    root->addWidget(constraintsList);

    body->setLayout(root);
    addWidget(body, true, false);

    if (sketchView) {
        sketchView->setTaskPanel(this);
    }

    refresh();
}

TaskSketcher3DTool::~TaskSketcher3DTool()
{
    if (sketchView) {
        sketchView->setTaskPanel(nullptr);
    }
}

void TaskSketcher3DTool::setHint(const QString& text)
{
    if (hintLabel) {
        hintLabel->setText(text);
    }
}

void TaskSketcher3DTool::refresh()
{
    populateElements();
    populateConstraints();
}

void TaskSketcher3DTool::populateElements()
{
    if (!elementsList) {
        return;
    }
    elementsList->clear();

    auto* sketch = sketchView ? sketchView->getSketch3DObject() : nullptr;
    if (!sketch) {
        elementsHeader->setText(tr("Elements"));
        return;
    }

    const auto fmtVec = [](const Base::Vector3d& v) {
        return QStringLiteral("(%1, %2, %3)")
            .arg(v.x, 0, 'f', 3)
            .arg(v.y, 0, 'f', 3)
            .arg(v.z, 0, 'f', 3);
    };
    const auto addRow = [this](const QString& text, const QString& subname) {
        auto* item = new QListWidgetItem(text, elementsList);
        item->setData(Qt::UserRole, subname);
    };

    const Part::TopoShape& shape = sketch->Shape.getShape();
    const auto& geos = sketch->Geometry.getValues();

    std::map<int, int> lineLabelForGeoId;
    std::map<int, int> pointLabelForGeoId;
    int lineLabelNext = 0;
    int pointLabelNext = 0;

    const unsigned long edgeCount = shape.countSubShapes(TopAbs_EDGE);
    for (unsigned long i = 1; i <= edgeCount; ++i) {
        TopoDS_Shape sub;
        try {
            sub = shape.getSubShape(TopAbs_EDGE, static_cast<int>(i), true);
        }
        catch (const Standard_Failure&) {
            continue;
        }
        if (sub.IsNull()) {
            continue;
        }

        int geoId = -1;
        Base::Vector3d a;
        Base::Vector3d b;
        for (std::size_t gi = 0; gi < geos.size(); ++gi) {
            const auto* ls = dynamic_cast<const Part::GeomLineSegment*>(geos[gi]);
            if (!ls) {
                continue;
            }
            geoId = static_cast<int>(gi);
            a = ls->getStartPoint();
            b = ls->getEndPoint();
            break;
        }
        const int label =
            (geoId >= 0 ? lineLabelForGeoId.emplace(geoId, ++lineLabelNext).first->second
                        : ++lineLabelNext);
        const QString subname = QStringLiteral("Edge%1").arg(static_cast<qulonglong>(i));
        addRow(tr("Line%1  length %2").arg(label).arg((b - a).Length(), 0, 'f', 3), subname);
    }

    const unsigned long vertexCount = shape.countSubShapes(TopAbs_VERTEX);
    for (unsigned long i = 1; i <= vertexCount; ++i) {
        TopoDS_Shape sub;
        try {
            sub = shape.getSubShape(TopAbs_VERTEX, static_cast<int>(i), true);
        }
        catch (const Standard_Failure&) {
            continue;
        }
        if (sub.IsNull()) {
            continue;
        }

        const auto id = sketch->resolvePickedVertex(TopoDS::Vertex(sub));
        if (!id.isValid()) {
            continue;
        }

        const gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(sub));
        const Base::Vector3d pos(p.X(), p.Y(), p.Z());
        const QString subname = QStringLiteral("Vertex%1").arg(static_cast<qulonglong>(i));

        QString text;
        if (id.Kind == Sketcher3D::GeoKind::Point) {
            const int label =
                pointLabelForGeoId.emplace(id.GeoId, ++pointLabelNext).first->second;
            text = tr("Point%1  %2").arg(label).arg(fmtVec(pos));
        }
        else if (id.Kind == Sketcher3D::GeoKind::Line) {
            const int label =
                lineLabelForGeoId.emplace(id.GeoId, ++lineLabelNext).first->second;
            text = id.Pos == Sketcher3D::PointPos::start
                ? tr("  Line%1.start  %2").arg(label).arg(fmtVec(pos))
                : tr("  Line%1.end  %2").arg(label).arg(fmtVec(pos));
        }
        addRow(text, subname);
    }

    elementsHeader->setText(tr("Elements (%1)").arg(elementsList->count()));
}

void TaskSketcher3DTool::populateConstraints()
{
    if (!constraintsList) {
        return;
    }
    constraintsList->clear();

    auto* sketch = sketchView ? sketchView->getSketch3DObject() : nullptr;
    if (!sketch) {
        constraintsHeader->setText(tr("Constraints"));
        return;
    }

    const auto& cs = sketch->Constraints.getConstraints();
    for (std::size_t i = 0; i < cs.size(); ++i) {
        const auto& c = cs[i];
        QString type = QString::fromUtf8(
            Sketcher3D::Constraint3D::typeToString(c.Type)
        );
        QStringList refs;
        for (const auto& r : c.getElements()) {
            refs << QStringLiteral("G%1").arg(r.GeoId);
        }
        QString text = tr("%1  [%2]").arg(type).arg(refs.join(QStringLiteral(", ")));
        new QListWidgetItem(text, constraintsList);
    }

    constraintsHeader->setText(tr("Constraints (%1)").arg(constraintsList->count()));
}

void TaskSketcher3DTool::onElementRowClicked(QListWidgetItem* item)
{
    if (!item || !sketchView) {
        return;
    }
    auto* sketch = sketchView->getSketch3DObject();
    if (!sketch || !sketch->getDocument()) {
        return;
    }
    const QString subname = item->data(Qt::UserRole).toString();
    if (subname.isEmpty()) {
        return;
    }

    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(
        sketch->getDocument()->getName(),
        sketch->getNameInDocument(),
        subname.toUtf8().constData()
    );
}

#include "moc_TaskSketcher3DTool.cpp"
