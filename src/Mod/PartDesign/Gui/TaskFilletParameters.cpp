// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#include <QAction>
#include <QListWidget>
#include <QMessageBox>
#include <QSignalBlocker>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>

#include <BRepCheck_Analyzer.hxx>
#include <Precision.hxx>

#include <Base/Interpreter.h>
#include <Base/Converter.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/PartDesign/App/FeatureFillet.h>
#include <Mod/Part/App/Attacher.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Part/App/GizmoHelper.h>

#include "ui_TaskFilletParameters.h"
#include "TaskFilletParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskFilletParameters */

TaskFilletParameters::TaskFilletParameters(ViewProviderDressUp* DressUpView, QWidget* parent)
    : TaskDressUpParameters(DressUpView, true, true, parent)
    , ui(new Ui_TaskFilletParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    PartDesign::Fillet* pcFillet = DressUpView->getObject<PartDesign::Fillet>();
    bool useAllEdges = pcFillet->UseAllEdges.getValue();
    ui->checkBoxUseAllEdges->setChecked(useAllEdges);
    ui->buttonRefSel->setEnabled(!useAllEdges);
    ui->listWidgetReferences->setEnabled(!useAllEdges);
    double r = pcFillet->Radius.getValue();
    lastValidRadius = r;

    ui->filletRadius->setUnit(Base::Unit::Length);
    ui->filletRadius->setValue(r);
    ui->filletRadius->setMinimum(Precision::Confusion());
    ui->filletRadius->selectNumber();
    ui->filletRadius->bind(pcFillet->Radius);
    QMetaObject::invokeMethod(ui->filletRadius, "setFocus", Qt::QueuedConnection);
    std::vector<std::string> strings = pcFillet->Base.getSubValues();
    for (const auto& string : strings) {
        ui->listWidgetReferences->addItem(QString::fromStdString(string));
    }

    QMetaObject::connectSlotsByName(this);

    // clang-format off
    connect(ui->filletRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
        this, &TaskFilletParameters::onLengthChanged);
    connect(ui->buttonRefSel, &QToolButton::toggled,
        this, &TaskFilletParameters::onButtonRefSel);
    connect(ui->checkBoxUseAllEdges, &QToolButton::toggled,
        this, &TaskFilletParameters::onCheckBoxUseAllEdgesToggled);

    // Create context menu
    createDeleteAction(ui->listWidgetReferences);
    connect(deleteAction, &QAction::triggered, this, &TaskFilletParameters::onRefDeleted);

    createAddAllEdgesAction(ui->listWidgetReferences);
    connect(addAllEdgesAction, &QAction::triggered, this, &TaskFilletParameters::onAddAllEdges);

    connect(ui->listWidgetReferences, &QListWidget::currentItemChanged,
        this, &TaskFilletParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemClicked,
        this, &TaskFilletParameters::setSelection);
    connect(ui->listWidgetReferences, &QListWidget::itemDoubleClicked,
        this, &TaskFilletParameters::doubleClicked);
    // clang-format on

    if (strings.empty()) {
        setSelectionMode(refSel);
    }
    else {
        hideOnError();
    }

    setupGizmos(DressUpView);
}

void TaskFilletParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // executed when the user selected something in the CAD object
    // adds/deletes the selection accordingly

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == refSel) {
            referenceSelected(msg, ui->listWidgetReferences);
            clearRadiusError();
        }
    }
    else if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        // TODO: the gizmo position should be only recalculated when the feature associated
        // with the gizmo is removed from the list
        setGizmoPositions();
        clearRadiusError();
    }
}

void TaskFilletParameters::onCheckBoxUseAllEdgesToggled(bool checked)
{
    if (auto fillet = getObject<PartDesign::Fillet>()) {
        if (checked) {
            setSelectionMode(none);
        }

        ui->buttonRefSel->setEnabled(!checked);
        ui->listWidgetReferences->setEnabled(!checked);
        fillet->UseAllEdges.setValue(checked);
        fillet->recomputeFeature();
        clearRadiusError();
    }
}

void TaskFilletParameters::setButtons(const selectionModes mode)
{
    ui->buttonRefSel->setChecked(mode == refSel);
    ui->buttonRefSel->setText(mode == refSel ? stopSelectionLabel() : startSelectionLabel());
}

void TaskFilletParameters::onRefDeleted()
{
    TaskDressUpParameters::deleteRef(ui->listWidgetReferences);
    setGizmoPositions();
    clearRadiusError();
}

void TaskFilletParameters::onAddAllEdges()
{
    TaskDressUpParameters::addAllEdges(ui->listWidgetReferences);
    clearRadiusError();
}

void TaskFilletParameters::onLengthChanged(double len)
{
    if (auto fillet = getObject<PartDesign::Fillet>()) {
        setSelectionMode(none);
        setupTransaction();

        double suggestedMaximum = 0.0;
        if (!isRadiusAllowed(len, &suggestedMaximum)) {
            QSignalBlocker blocker(ui->filletRadius);
            std::optional<double> maximumRadius;
            if (suggestedMaximum > lastValidRadius && suggestedMaximum < len) {
                const double tolerance = std::max(Precision::Confusion(), std::abs(len) * 1.0e-9);
                const double rounded = std::round(suggestedMaximum / tolerance) * tolerance;
                if (rounded <= len && isRadiusAllowed(rounded)) {
                    maximumRadius = rounded;
                }
                else if (isRadiusAllowed(suggestedMaximum)) {
                    maximumRadius = suggestedMaximum;
                }
            }
            if (!maximumRadius) {
                maximumRadius = findMaximumAllowedRadius(len);
            }
            const double restoredRadius = maximumRadius.value_or(lastValidRadius);
            lastValidRadius = restoredRadius;
            fillet->Radius.setValue(restoredRadius);
            ui->filletRadius->setValue(restoredRadius);
            fillet->recomputeFeature();
            if (maximumRadius) {
                const QString maximumText = QString::fromStdString(
                    Base::Quantity(*maximumRadius, Base::Unit::Length).getUserString()
                );
                ui->radiusErrorLabel->setText(
                    tr("The requested radius exceeds the maximum permitted radius of %1. "
                       "The radius has been set to this maximum.")
                        .arg(maximumText)
                );
            }
            else {
                ui->radiusErrorLabel->setText(
                    tr("This radius is not permitted by the selected geometry. The previous "
                       "valid value has been restored.")
                );
            }
            ui->radiusErrorLabel->show();
            hideOnError();
            return;
        }

        lastValidRadius = len;
        clearRadiusError();
        fillet->Radius.setValue(len);
        fillet->recomputeFeature();
        // hide the fillet if there was a computation error
        hideOnError();
    }
}

std::optional<double> TaskFilletParameters::findMaximumAllowedRadius(double requestedRadius) const
{
    // Only infer an upper bound when the user increased a currently valid
    // radius. Other failures need not be monotonic and retain the previous
    // value instead.
    if (requestedRadius <= lastValidRadius || !isRadiusAllowed(lastValidRadius)) {
        return std::nullopt;
    }

    double lower = lastValidRadius;
    double upper = requestedRadius;
    const double tolerance = std::max(Precision::Confusion(), std::abs(requestedRadius) * 1.0e-9);
    for (int iteration = 0; iteration < 50 && upper - lower > tolerance; ++iteration) {
        const double trial = (lower + upper) * 0.5;
        if (isRadiusAllowed(trial)) {
            lower = trial;
        }
        else {
            upper = trial;
        }
    }

    // Prefer the exact tolerance-rounded limit where it is supported. This
    // preserves exact face-consuming cases such as 5 mm instead of retaining
    // an artificial 4.999999... radius.
    const double rounded = std::round(lower / tolerance) * tolerance;
    if (rounded > lower && rounded <= requestedRadius && isRadiusAllowed(rounded)) {
        lower = rounded;
    }
    return lower;
}

bool TaskFilletParameters::isRadiusAllowed(double radius, double* maximumRadius) const
{
    if (radius <= Precision::Confusion()) {
        return false;
    }

    auto fillet = getObject<PartDesign::Fillet>();
    if (!fillet) {
        return false;
    }

    try {
        Part::TopoShape baseShape = fillet->getBaseTopoShape(true);
        if (baseShape.isNull()) {
            return true;
        }
        baseShape.setTransform(Base::Matrix4D());

        const auto edges = fillet->UseAllEdges.getValue() ? baseShape.getSubTopoShapes(TopAbs_EDGE)
                                                          : fillet->getContinuousEdges(baseShape);
        if (edges.empty()) {
            return true;
        }

        // Radius probing does not need element-name history. Avoid repeatedly
        // constructing that history during the upper-bound search.
        Part::TopoShape geometry(baseShape.getShape());
        std::vector<Part::TopoShape> geometryEdges;
        geometryEdges.reserve(edges.size());
        for (const auto& edge : edges) {
            geometryEdges.emplace_back(edge.getShape());
        }
        Part::TopoShape result;
        result.makeElementFillet(geometry, geometryEdges, radius, radius);
        return !result.isNull() && BRepCheck_Analyzer(result.getShape()).IsValid();
    }
    catch (const Base::CADKernelError& error) {
        if (maximumRadius) {
            constexpr std::string_view marker = "exceeds the approximately ";
            const std::string_view message(error.what());
            const std::size_t markerPosition = message.find(marker);
            if (markerPosition != std::string_view::npos) {
                const std::string numberText(message.substr(markerPosition + marker.size()));
                char* end = nullptr;
                const double parsedMaximum = std::strtod(numberText.c_str(), &end);
                if (end != numberText.c_str() && parsedMaximum > 0.0) {
                    *maximumRadius = parsedMaximum;
                }
            }
        }
        return false;
    }
    catch (...) {
        return false;
    }
}

void TaskFilletParameters::clearRadiusError()
{
    ui->radiusErrorLabel->hide();
}

double TaskFilletParameters::getLength() const
{
    return ui->filletRadius->value().getValue();
}

TaskFilletParameters::~TaskFilletParameters()
{
    try {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
    }
    catch (const Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }
}

void TaskFilletParameters::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskFilletParameters::apply()
{
    ui->filletRadius->apply();

    // Alert user if he created an empty feature
    if (ui->listWidgetReferences->count() == 0) {
        std::string text = tr("Empty fillet created!").toStdString();
        Base::Console().warning("%s\n", text.c_str());
    }
}

void TaskFilletParameters::setupGizmos(ViewProviderDressUp* vp)
{
    if (!GizmoContainer::isEnabled()) {
        return;
    }

    radiusGizmo = new Gui::LinearGizmo(ui->filletRadius);
    radiusGizmo2 = new Gui::LinearGizmo(ui->filletRadius);

    gizmoContainer = GizmoContainer::create({radiusGizmo, radiusGizmo2}, vp);

    setGizmoPositions();
    showDraggerHints();
}

void TaskFilletParameters::setGizmoPositions()
{
    if (!gizmoContainer) {
        return;
    }

    auto fillet = getObject<PartDesign::Fillet>();
    if (!fillet || fillet->isError()) {
        gizmoContainer->visible = false;
        return;
    }
    Part::TopoShape baseShape = fillet->getBaseTopoShape(true);
    std::vector<Part::TopoShape> shapes = fillet->getContinuousEdges(baseShape);

    if (shapes.size() == 0) {
        gizmoContainer->visible = false;
        return;
    }
    gizmoContainer->visible = true;

    // Attach the arrow to the first edge
    Part::TopoShape edge = shapes[0];
    auto [face1, face2] = getAdjacentFacesFromEdge(edge, baseShape);

    DraggerPlacementProps props1 = getDraggerPlacementFromEdgeAndFace(edge, face1);
    radiusGizmo->Gizmo::setDraggerPlacement(props1.position, props1.dir);

    DraggerPlacementProps props2 = getDraggerPlacementFromEdgeAndFace(edge, face2);
    radiusGizmo2->Gizmo::setDraggerPlacement(props2.position, props2.dir);

    // The dragger length won't be equal to the radius if the two faces
    // are not orthogonal so this correction is needed
    double angle = props1.dir.GetAngle(props2.dir);
    double correction = 1 / std::tan(angle / 2);

    radiusGizmo->setMultFactor(correction);
    radiusGizmo2->setMultFactor(correction);
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFilletParameters::TaskDlgFilletParameters(ViewProviderFillet* DressUpView)
    : TaskDlgDressUpParameters(DressUpView)
{
    parameter = new TaskFilletParameters(DressUpView);

    Content.push_back(parameter);
    Content.push_back(preview);
}

TaskDlgFilletParameters::~TaskDlgFilletParameters() = default;

//==== calls from the TaskView ===============================================================

bool TaskDlgFilletParameters::accept()
{
    auto obj = getObject();
    if (!obj->isError()) {
        getViewObject()->showPreviousFeature(false);
    }

    parameter->apply();

    return TaskDlgDressUpParameters::accept();
}

#include "moc_TaskFilletParameters.cpp"
