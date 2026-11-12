// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <QGraphicsPathItem>
#include <QPainterPath>

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>

#include "DrawGuiUtil.h"
#include "PreferencesGui.h"
#include "QGISketch.h"
#include "Rez.h"
#include "ZVALUE.h"

using namespace TechDrawGui;

namespace
{
QPointF toPagePoint(const gp_Pnt& point, TechDraw::DrawView* owner)
{
    if (owner) {
        const Base::Vector3d sketchPoint(point.X(), point.Y(), 0.0);
        const Base::Vector3d converted = DrawGuiUtil::toGuiPoint(owner, sketchPoint);
        return {converted.x, converted.y};
    }
    return {Rez::guiX(point.X()), -Rez::guiX(point.Y())};
}
}

QGISketch::QGISketch(App::DocumentObject* sketch, TechDraw::DrawView* owner)
    : m_sketch(sketch),
      m_owner(owner),
      m_geometry(new QGraphicsPathItem(this)),
      m_normalPen(PreferencesGui::normalQColor()),
      m_selectedPen(PreferencesGui::selectQColor())
{
    setData(0, QStringLiteral("QGISketch"));
    if (m_sketch && m_sketch->getNameInDocument()) {
        setData(1, QString::fromUtf8(m_sketch->getNameInDocument()));
    }

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setHandlesChildEvents(true);
    setZValue(ZVALUE::EDGE);

    m_geometry->setBrush(Qt::NoBrush);

    updateView();
}

App::DocumentObject* QGISketch::getSketchObject() const
{
    return m_sketch;
}

TechDraw::DrawView* QGISketch::getOwnerView() const
{
    return m_owner;
}

void QGISketch::setOwnerView(TechDraw::DrawView* owner)
{
    m_owner = owner;
    updateView();
}

void QGISketch::updateView()
{
    updatePens();

    QPainterPath path;
    auto* feature = dynamic_cast<Part::Feature*>(m_sketch);
    if (!feature || !TechDraw::DrawPage::isSketch(feature)) {
        m_geometry->setPath(path);
        return;
    }

    const TopoDS_Shape shape = feature->Shape.getValue();
    for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
        const TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        BRepAdaptor_Curve curve(edge);

        if (curve.GetType() == GeomAbs_Line) {
            path.moveTo(toPagePoint(curve.Value(curve.FirstParameter()), m_owner));
            path.lineTo(toPagePoint(curve.Value(curve.LastParameter()), m_owner));
            continue;
        }

        GCPnts_UniformDeflection discretizer(curve, 0.05);
        if (!discretizer.IsDone() || discretizer.NbPoints() == 0) {
            continue;
        }

        for (int i = 1; i <= discretizer.NbPoints(); ++i) {
            const QPointF point =
                toPagePoint(curve.Value(discretizer.Parameter(i)), m_owner);
            if (i == 1) {
                path.moveTo(point);
            }
            else {
                path.lineTo(point);
            }
        }
    }

    m_geometry->setPath(path);
}

void QGISketch::updatePens()
{
    QColor normalColor = PreferencesGui::normalQColor();
    qreal width = 1.0;
    Qt::PenStyle style = Qt::SolidLine;

    Gui::ViewProvider* viewProvider = m_sketch
        ? Gui::Application::Instance->getViewProvider(m_sketch)
        : nullptr;

    if (viewProvider) {
        if (auto* lineWidth = dynamic_cast<App::PropertyFloat*>(
                viewProvider->getPropertyByName("LineWidth"))) {
            width = lineWidth->getValue();
        }

        if (auto* drawStyle = dynamic_cast<App::PropertyEnumeration*>(
                viewProvider->getPropertyByName("DrawStyle"))) {
            // ViewProviderPartExt uses the same order as Qt: solid, dash,
            // dot and dash-dot, with Qt::SolidLine starting at 1.
            const int qtStyle = drawStyle->getValue() + static_cast<int>(Qt::SolidLine);
            if (qtStyle >= static_cast<int>(Qt::SolidLine)
                && qtStyle <= static_cast<int>(Qt::DashDotLine)) {
                style = static_cast<Qt::PenStyle>(qtStyle);
            }
        }

        auto* autoColor = dynamic_cast<App::PropertyBool*>(
            viewProvider->getPropertyByName("AutoColor"));
        if ((!autoColor || !autoColor->getValue())) {
            if (auto* lineColor = dynamic_cast<App::PropertyColor*>(
                    viewProvider->getPropertyByName("LineColor"))) {
                normalColor = lineColor->getValue().asValue<QColor>();
            }
        }
    }

    m_normalPen = QPen(normalColor, width, style);
    m_normalPen.setCosmetic(true);
    m_selectedPen = QPen(PreferencesGui::selectQColor(), width, style);
    m_selectedPen.setCosmetic(true);
    m_geometry->setPen(isSelected() ? m_selectedPen : m_normalPen);
}

void QGISketch::setGroupSelection(bool selected)
{
    setSelected(selected);
}

QVariant QGISketch::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged) {
        m_geometry->setPen(value.toBool() ? m_selectedPen : m_normalPen);
    }
    return QGraphicsItemGroup::itemChange(change, value);
}
