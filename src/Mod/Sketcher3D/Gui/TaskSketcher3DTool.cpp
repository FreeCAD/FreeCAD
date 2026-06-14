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
#include <QEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
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
#include <Mod/Sketcher3D/App/Constraint3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "TaskSketcher3DTool.h"
#include "ViewProviderSketch3D.h"


using namespace Sketcher3DGui;

namespace
{

QString planeName(ViewProviderSketch3D::ActivePlane plane)
{
    switch (plane) {
        case ViewProviderSketch3D::ActivePlane::YZ:
            return QStringLiteral("YZ");
        case ViewProviderSketch3D::ActivePlane::ZX:
            return QStringLiteral("ZX");
        case ViewProviderSketch3D::ActivePlane::XY:
        default:
            return QStringLiteral("XY");
    }
}

QString posName(Sketcher3D::PointPos pos)
{
    switch (pos) {
        case Sketcher3D::PointPos::start:
            return QStringLiteral(".start");
        case Sketcher3D::PointPos::end:
            return QStringLiteral(".end");
        case Sketcher3D::PointPos::mid:
            return QStringLiteral(".mid");
        case Sketcher3D::PointPos::none:
        default:
            return QString();
    }
}

void buildGeometryLabels(
    const std::vector<Part::Geometry*>& geos,
    std::map<int, int>& pointLabels,
    std::map<int, int>& lineLabels
)
{
    int pointNext = 0;
    int lineNext = 0;
    for (std::size_t i = 0; i < geos.size(); ++i) {
        const int geoId = static_cast<int>(i);
        const Part::Geometry* geo = geos[i];
        if (!geo) {
            continue;
        }
        if (geo->is<Part::GeomPoint>()) {
            pointLabels.emplace(geoId, ++pointNext);
        }
        else if (geo->is<Part::GeomLineSegment>()) {
            lineLabels.emplace(geoId, ++lineNext);
        }
    }
}

QString displayNameForRef(
    const Sketcher3D::GeoElementId3D& ref,
    const std::map<int, int>& pointLabels,
    const std::map<int, int>& lineLabels
)
{
    if (auto it = pointLabels.find(ref.GeoId); it != pointLabels.end()) {
        return QStringLiteral("Point%1").arg(it->second);
    }
    if (auto it = lineLabels.find(ref.GeoId); it != lineLabels.end()) {
        return QStringLiteral("Line%1%2").arg(it->second).arg(posName(ref.Pos));
    }
    return QStringLiteral("G%1%2").arg(ref.GeoId).arg(posName(ref.Pos));
}

QString subNameForRef(const Sketcher3D::Sketch3DObject* sketch, const Sketcher3D::GeoElementId3D& ref)
{
    if (!sketch || !ref.isValid()) {
        return {};
    }

    const Part::TopoShape& shape = sketch->Shape.getShape();
    if (ref.Pos == Sketcher3D::PointPos::none && ref.Kind != Sketcher3D::GeoKind::Point) {
        const unsigned long edgeCount = shape.countSubShapes(TopAbs_EDGE);
        for (unsigned long i = 1; i <= edgeCount; ++i) {
            TopoDS_Shape sub;
            try {
                sub = shape.getSubShape(TopAbs_EDGE, static_cast<int>(i), true);
            }
            catch (const Standard_Failure&) {
                continue;
            }
            if (sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) {
                continue;
            }
            const QString subname = QStringLiteral("Edge%1").arg(static_cast<qulonglong>(i));
            const auto id = sketch->resolveSubName(subname.toStdString());
            if (id == ref) {
                return subname;
            }
        }
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
        if (sub.IsNull() || sub.ShapeType() != TopAbs_VERTEX) {
            continue;
        }
        const QString subname = QStringLiteral("Vertex%1").arg(static_cast<qulonglong>(i));
        const auto id = sketch->resolveSubName(subname.toStdString());
        if (id == ref) {
            return subname;
        }
    }

    return {};
}

QIcon iconForConstraint(Sketcher3D::Constraint3D::Constraint3DType type)
{
    switch (type) {
        case Sketcher3D::Constraint3D::Coincident3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_PointOnPoint");
        case Sketcher3D::Constraint3D::Parallel3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Parallel");
        case Sketcher3D::Constraint3D::Angle3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle");
        case Sketcher3D::Constraint3D::AlongX:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Horizontal");
        case Sketcher3D::Constraint3D::AlongY:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Vertical");
        case Sketcher3D::Constraint3D::AlongZ:
            return Gui::BitmapFactory().iconFromTheme("Std_Axis");
        case Sketcher3D::Constraint3D::DistanceX3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance");
        case Sketcher3D::Constraint3D::DistanceY3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance");
        case Sketcher3D::Constraint3D::DistanceZ3D:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Length");
        case Sketcher3D::Constraint3D::Distance3D:
        default:
            return Gui::BitmapFactory().iconFromTheme("Constraint_Length");
    }
}

}  // namespace

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
    setHint(tr("Pick a tool from the toolbar, then click in the 3D view."));
    root->addWidget(hintLabel);

    statusLabel = new QLabel();
    statusLabel->setWordWrap(true);
    root->addWidget(statusLabel);

    planeLabel = new QLabel();
    root->addWidget(planeLabel);

    elementsHeader = new QLabel();
    root->addWidget(elementsHeader);

    elementsList = new QListWidget();
    elementsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    elementsList->setUniformItemSizes(true);
    elementsList->setAlternatingRowColors(true);
    elementsList->setMinimumHeight(100);
    connect(elementsList, &QListWidget::itemClicked, this, &TaskSketcher3DTool::onElementRowClicked);
    root->addWidget(elementsList);

    constraintsHeader = new QLabel();
    root->addWidget(constraintsHeader);

    constraintsList = new QListWidget();
    constraintsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    constraintsList->setUniformItemSizes(true);
    constraintsList->setAlternatingRowColors(true);
    constraintsList->setMinimumHeight(80);
    connect(constraintsList, &QListWidget::itemClicked, this, &TaskSketcher3DTool::onConstraintRowClicked);
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
    populateStatus();
    populateElements();
    populateConstraints();
}

void TaskSketcher3DTool::populateStatus()
{
    auto* sketch = sketchView ? sketchView->getSketch3DObject() : nullptr;
    if (!sketch) {
        if (statusLabel) {
            statusLabel->setText(tr("No active 3D sketch"));
        }
        if (planeLabel) {
            planeLabel->clear();
        }
        return;
    }

    int pointCount = 0;
    int lineCount = 0;
    int otherCount = 0;
    for (const Part::Geometry* geo : sketch->Geometry.getValues()) {
        if (!geo) {
            continue;
        }
        if (geo->is<Part::GeomPoint>()) {
            ++pointCount;
        }
        else if (geo->is<Part::GeomLineSegment>()) {
            ++lineCount;
        }
        else {
            ++otherCount;
        }
    }

    const int constraintCount = sketch->Constraints.getSize();
    QStringList parts;
    parts << tr("%1 point(s)").arg(pointCount);
    parts << tr("%1 line(s)").arg(lineCount);
    if (otherCount > 0) {
        parts << tr("%1 other").arg(otherCount);
    }

    if (statusLabel) {
        statusLabel->setText(
            tr("%1 constraint(s) | %2").arg(constraintCount).arg(parts.join(QStringLiteral(", ")))
        );
    }
    if (planeLabel && sketchView) {
        const Base::Vector3d& base = sketchView->getPlaneBase();
        planeLabel->setText(tr("Active plane: %1 at (%2, %3, %4)")
                                .arg(planeName(sketchView->getActivePlane()))
                                .arg(base.x, 0, 'f', 3)
                                .arg(base.y, 0, 'f', 3)
                                .arg(base.z, 0, 'f', 3));
    }
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
        return QStringLiteral("(%1, %2, %3)").arg(v.x, 0, 'f', 3).arg(v.y, 0, 'f', 3).arg(v.z, 0, 'f', 3);
    };
    const auto addRow = [this](const QString& text, const QString& subname, const QIcon& icon) {
        auto* item = new QListWidgetItem(text, elementsList);
        item->setIcon(icon);
        item->setData(Qt::UserRole, subname);
    };

    const Part::TopoShape& shape = sketch->Shape.getShape();
    const auto& geos = sketch->Geometry.getValues();

    std::map<int, int> lineLabelForGeoId;
    std::map<int, int> pointLabelForGeoId;
    buildGeometryLabels(geos, pointLabelForGeoId, lineLabelForGeoId);

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

        const QString subname = QStringLiteral("Edge%1").arg(static_cast<qulonglong>(i));
        const auto id = sketch->resolveSubName(subname.toStdString());
        if (!id.isValid()) {
            continue;
        }
        if (id.GeoId < 0 || id.GeoId >= static_cast<int>(geos.size())) {
            continue;
        }
        const auto* ls = dynamic_cast<const Part::GeomLineSegment*>(geos[id.GeoId]);
        if (!ls) {
            continue;
        }
        const int label = lineLabelForGeoId[id.GeoId];
        const double length = (ls->getEndPoint() - ls->getStartPoint()).Length();
        addRow(
            tr("Line%1  length %2").arg(label).arg(length, 0, 'f', 3),
            subname,
            Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine")
        );
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

        const QString subname = QStringLiteral("Vertex%1").arg(static_cast<qulonglong>(i));
        const auto id = sketch->resolveSubName(subname.toStdString());
        if (!id.isValid()) {
            continue;
        }

        const gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(sub));
        const Base::Vector3d pos(p.X(), p.Y(), p.Z());

        QString text;
        if (id.Kind == Sketcher3D::GeoKind::Point) {
            const int label = pointLabelForGeoId[id.GeoId];
            text = tr("Point%1  %2").arg(label).arg(fmtVec(pos));
        }
        else if (id.Kind == Sketcher3D::GeoKind::Line) {
            const int label = lineLabelForGeoId[id.GeoId];
            text = id.Pos == Sketcher3D::PointPos::start
                ? tr("  Line%1.start  %2").arg(label).arg(fmtVec(pos))
                : tr("  Line%1.end  %2").arg(label).arg(fmtVec(pos));
        }
        if (!text.isEmpty()) {
            addRow(text, subname, Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePoint"));
        }
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
    std::map<int, int> lineLabelForGeoId;
    std::map<int, int> pointLabelForGeoId;
    buildGeometryLabels(sketch->Geometry.getValues(), pointLabelForGeoId, lineLabelForGeoId);

    for (std::size_t i = 0; i < cs.size(); ++i) {
        const auto& c = cs[i];
        QString type = QString::fromUtf8(Sketcher3D::Constraint3D::typeToString(c.Type));
        QStringList refs;
        QStringList subnames;
        for (const auto& r : c.getElements()) {
            refs << displayNameForRef(r, pointLabelForGeoId, lineLabelForGeoId);
            const QString subname = subNameForRef(sketch, r);
            if (!subname.isEmpty()) {
                subnames << subname;
            }
        }
        QString text = tr("%1  %2  [%3]")
                           .arg(static_cast<int>(i + 1))
                           .arg(type)
                           .arg(refs.join(QStringLiteral(", ")));
        if (c.Type == Sketcher3D::Constraint3D::Distance3D
            || c.Type == Sketcher3D::Constraint3D::DistanceX3D
            || c.Type == Sketcher3D::Constraint3D::DistanceY3D
            || c.Type == Sketcher3D::Constraint3D::DistanceZ3D) {
            text += tr(" = %1").arg(c.Value, 0, 'f', 3);
        }
        else if (c.Type == Sketcher3D::Constraint3D::Angle3D) {
            text += tr(" = %1 deg").arg(Base::toDegrees(c.Value), 0, 'f', 3);
        }
        auto* item = new QListWidgetItem(iconForConstraint(c.Type), text, constraintsList);
        item->setData(Qt::UserRole, subnames);
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

    QStringList subnames;
    subnames << subname;
    selectSubNames(subnames);
}

void TaskSketcher3DTool::onConstraintRowClicked(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    selectSubNames(item->data(Qt::UserRole).toStringList());
}

void TaskSketcher3DTool::selectSubNames(const QStringList& subnames) const
{
    auto* sketch = sketchView ? sketchView->getSketch3DObject() : nullptr;
    if (!sketch || !sketch->getDocument()) {
        return;
    }

    Gui::Selection().clearSelection();
    for (const QString& subname : subnames) {
        if (subname.isEmpty()) {
            continue;
        }
        Gui::Selection().addSelection(
            sketch->getDocument()->getName(),
            sketch->getNameInDocument(),
            subname.toUtf8().constData()
        );
    }
}

#include "moc_TaskSketcher3DTool.cpp"
