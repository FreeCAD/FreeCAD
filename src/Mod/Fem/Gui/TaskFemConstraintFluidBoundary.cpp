/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
 *   Copyright (c) 2016 Qingfeng Xia <qingfeng.xia[at]iesensor.com>        *
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
#include <QAction>
#include <QMessageBox>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <sstream>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Command.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/Fem/App/FemAnalysis.h>
#include <Mod/Fem/App/FemConstraintFluidBoundary.h>
#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemSolverObject.h>
#include <Mod/Fem/App/FemTools.h>

#include "ActiveAnalysisObserver.h"
#include "TaskFemConstraintFluidBoundary.h"
#include "ui_TaskFemConstraintFluidBoundary.h"


using namespace FemGui;
using namespace Gui;
using namespace Fem;

// also defined in FemConstrainFluidBoundary and foamcasebuilder/basicbuilder.py, please update
// simultaneously the second (index 1) is the default enum, as index 0 causes compiling error static
// const char* BoundaryTypes[] = {"inlet","wall","outlet","freestream", "interface", NULL};
static const char* WallSubtypes[] =
    {"unspecific", "fixed", "slip", "partialSlip", "moving", "rough", nullptr};
static const char* InletSubtypes[] = {"unspecific",
                                      "totalPressure",
                                      "uniformVelocity",
                                      "volumetricFlowRate",
                                      "massFlowRate",
                                      nullptr};
static const char* OutletSubtypes[] =
    {"unspecific", "totalPressure", "staticPressure", "uniformVelocity", "outFlow", nullptr};
static const char* InterfaceSubtypes[] =
    {"unspecific", "symmetry", "wedge", "cyclic", "empty", "coupled", nullptr};
static const char* FreestreamSubtypes[] = {"unspecific", "freestream", nullptr};

static const char* InterfaceSubtypeHelpTexts[] = {
    "invalid,select other valid interface subtype",
    "symmetry plane but not axis-sym axis line",
    "axis symmetric front and back surfaces",
    "periodic boundary in pair, treated as physical connected",
    "front and back for single layer 2D mesh, also axis-sym axis line",
    "exchange boundary vale with external program, need extra manual setup like file name",
    nullptr};

// defined in file FemConstraintFluidBoundary:
// see Ansys fluet manual: Turbulence Specification method
// static const char* TurbulenceSpecifications[] = {"intensity&DissipationRate",
// "intensity&LengthScale","intensity&ViscosityRatio", "intensity&HydraulicDiameter",NULL}; activate
// the heat transfer and radiation model in Solver object explorer
static const char* TurbulenceSpecificationHelpTexts[] = {
    "explicitly specific intensity k [SI unit] and dissipation rate epsilon [] / omega []",
    "intensity (0.05 ~ 0.15) and characteristic length scale of max eddy [m]",
    "intensity (0.05 ~ 0.15) and turbulent viscosity ratio",
    "for fully developed internal flow, Turbulence intensity (0-1.0) 0.05 typical",
    nullptr};

// static const char* ThermalBoundaryTypes[] = {"fixedValue","zeroGradient", "fixedGradient",
// "mixed", "heatFlux", "HTC","coupled", NULL};
static const char* ThermalBoundaryHelpTexts[] = {"fixed Temperature [K]",
                                                 "no heat transfer on boundary",
                                                 "fixed value gradient [K/m]",
                                                 "mixed fixedGradient and fixedValue",
                                                 "fixed heat flux [W/m2]",
                                                 "Heat transfer coeff [W/(M2)/K]",
                                                 "conjugate heat transfer with solid",
                                                 nullptr};
// enable & disable quantityUI once valueType is selected

// internal function not declared in header file
void initComboBox(QComboBox* combo,
                  const std::vector<std::string>& textItems,
                  const std::string& sItem)
{
    combo->blockSignals(true);

    int iItem = 1;  // the first one is "unspecific" (index 0)
    combo->clear();
    for (unsigned int it = 0; it < textItems.size(); it++) {
        combo->insertItem(it, Base::Tools::fromStdString(textItems[it]));
        if (sItem == textItems[it]) {
            iItem = it;
        }
    }
    combo->setCurrentIndex(iItem);
    combo->blockSignals(false);
}

/* TRANSLATOR FemGui::TaskFemConstraintFluidBoundary */
TaskFemConstraintFluidBoundary::TaskFemConstraintFluidBoundary(
    ViewProviderFemConstraintFluidBoundary* ConstraintView,
    QWidget* parent)
    : TaskFemConstraintOnBoundary(ConstraintView, parent, "FEM_ConstraintFluidBoundary")
    , ui(new Ui_TaskFemConstraintFluidBoundary)
    , dimension(-1)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // create a context menu for the listview of the references
    createDeleteAction(ui->listReferences);
    connect(deleteAction,
            &QAction::triggered,
            this,
            &TaskFemConstraintFluidBoundary::onReferenceDeleted);

    // setup ranges
    ui->spinBoundaryValue->setMinimum(-FLOAT_MAX);
    ui->spinBoundaryValue->setMaximum(FLOAT_MAX);
    ui->spinTurbulentIntensityValue->setMinimum(0.0);
    ui->spinTurbulentIntensityValue->setMaximum(FLOAT_MAX);
    ui->spinTurbulentLengthValue->setMinimum(0.0);
    ui->spinTurbulentLengthValue->setMaximum(FLOAT_MAX);
    ui->spinTemperatureValue->setMinimum(-273.15);
    ui->spinTemperatureValue->setMaximum(FLOAT_MAX);
    ui->spinHeatFluxValue->setMinimum(0.0);
    ui->spinHeatFluxValue->setMaximum(FLOAT_MAX);
    ui->spinHTCoeffValue->setMinimum(0.0);
    ui->spinHTCoeffValue->setMaximum(FLOAT_MAX);

    connect(ui->comboBoundaryType,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskFemConstraintFluidBoundary::onBoundaryTypeChanged);
    connect(ui->comboSubtype,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskFemConstraintFluidBoundary::onSubtypeChanged);
    connect(ui->spinBoundaryValue,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &TaskFemConstraintFluidBoundary::onBoundaryValueChanged);

    connect(ui->comboTurbulenceSpecification,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskFemConstraintFluidBoundary::onTurbulenceSpecificationChanged);
    connect(ui->comboThermalBoundaryType,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskFemConstraintFluidBoundary::onThermalBoundaryTypeChanged);

    connect(ui->buttonDirection, &QPushButton::pressed, this, [=] {
        onButtonDirection(true);
    });
    connect(ui->checkReverse,
            &QCheckBox::toggled,
            this,
            &TaskFemConstraintFluidBoundary::onCheckReverse);

    connect(ui->listReferences,
            &QListWidget::itemClicked,
            this,
            &TaskFemConstraintFluidBoundary::setSelection);

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinBoundaryValue->blockSignals(true);
    ui->listReferences->blockSignals(true);
    // boundaryType and subType combo signal is Temporarily prevented in initComboBox()
    ui->buttonDirection->blockSignals(true);
    ui->checkReverse->blockSignals(true);

    // Selection buttons
    buttonGroup->addButton(ui->btnAdd, (int)SelectionChangeModes::refAdd);
    buttonGroup->addButton(ui->btnRemove, (int)SelectionChangeModes::refRemove);

    // Get the feature data
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());

    Fem::FemAnalysis* pcAnalysis = nullptr;
    if (FemGui::ActiveAnalysisObserver::instance()->hasActiveObject()) {
        pcAnalysis = FemGui::ActiveAnalysisObserver::instance()->getActiveObject();
    }
    else {
        App::Document* aDoc = pcConstraint->getDocument();
        std::vector<App::DocumentObject*> fem =
            aDoc->getObjectsOfType(Fem::FemAnalysis::getClassTypeId());
        if (!fem.empty()) {
            pcAnalysis = static_cast<Fem::FemAnalysis*>(fem[0]);  // get the first
        }
    }

    Fem::FemMeshObject* pcMesh = nullptr;
    if (pcAnalysis) {
        std::vector<App::DocumentObject*> fem = pcAnalysis->Group.getValues();
        for (auto it : fem) {
            if (it->getTypeId().isDerivedFrom(Fem::FemMeshObject::getClassTypeId())) {
                pcMesh = static_cast<Fem::FemMeshObject*>(it);
            }
        }
    }
    else {
        Base::Console().Log("FemAnalysis object is not activated or no FemAnalysis in the active "
                            "document, mesh dimension is unknown\n");
        dimension = -1;  // unknown dimension of mesh
    }
    if (pcMesh) {
        App::Property* prop = pcMesh->getPropertyByName("Shape");  // PropertyLink
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
            App::PropertyLink* pcLink = static_cast<App::PropertyLink*>(prop);
            Part::Feature* pcPart = dynamic_cast<Part::Feature*>(pcLink->getValue());
            if (pcPart) {  // deduct dimension from part_obj.Shape.ShapeType
                const TopoDS_Shape& pShape = pcPart->Shape.getShape().getShape();
                const TopAbs_ShapeEnum shapeType =
                    pShape.IsNull() ? TopAbs_SHAPE : pShape.ShapeType();
                if (shapeType == TopAbs_SOLID
                    || shapeType == TopAbs_COMPSOLID) {  // COMPSOLID is solids connected by faces
                    dimension = 3;
                }
                else if (shapeType == TopAbs_FACE || shapeType == TopAbs_SHELL) {
                    dimension = 2;
                }
                else if (shapeType == TopAbs_EDGE || shapeType == TopAbs_WIRE) {
                    dimension = 1;
                }
                else {
                    dimension =
                        -1;  // Vertex (0D) can not make mesh, Compound type might contain any types
                }
            }
        }
    }

    pcSolver = nullptr;  // this is an private object of type Fem::FemSolverObject*
    if (pcAnalysis) {
        std::vector<App::DocumentObject*> fem = pcAnalysis->Group.getValues();
        for (auto it : fem) {
            if (it->getTypeId().isDerivedFrom(Fem::FemSolverObject::getClassTypeId())) {
                pcSolver = static_cast<Fem::FemSolverObject*>(it);
            }
        }
    }

    pHeatTransferring = nullptr;
    pTurbulenceModel = nullptr;
    if (pcSolver) {
        // if only it is CFD solver, otherwise exit by SIGSEGV error, detect getPropertyByName() !=
        // NULL
        if (pcSolver->getPropertyByName("HeatTransferring")) {
            pHeatTransferring =
                static_cast<App::PropertyBool*>(pcSolver->getPropertyByName("HeatTransferring"));
            if (pHeatTransferring->getValue()) {
                ui->tabThermalBoundary->setEnabled(true);
                initComboBox(ui->comboThermalBoundaryType,
                             pcConstraint->ThermalBoundaryType.getEnumVector(),
                             pcConstraint->ThermalBoundaryType.getValueAsString());
                ui->spinHTCoeffValue->setValue(pcConstraint->HTCoeffValue.getValue());
                ui->spinHeatFluxValue->setValue(pcConstraint->HeatFluxValue.getValue());
                ui->spinTemperatureValue->setValue(pcConstraint->TemperatureValue.getValue());
                updateThermalBoundaryUI();
            }
            else {
                ui->tabThermalBoundary->setEnabled(false);  // could be hidden
                // Base::Console().Message("retrieve solver property HeatTransferring as false\n");
            }
        }
        else {
            ui->tabThermalBoundary->setEnabled(false);
        }
        if (pcSolver->getPropertyByName("TurbulenceModel")) {
            pTurbulenceModel = static_cast<App::PropertyEnumeration*>(
                pcSolver->getPropertyByName("TurbulenceModel"));
            if (pTurbulenceModel->getValueAsString() == std::string("laminar")) {
                ui->tabTurbulenceBoundary->setEnabled(false);
            }
            else {
                ui->tabTurbulenceBoundary->setEnabled(true);
                ui->labelTurbulenceSpecification->setText(
                    Base::Tools::fromStdString(pTurbulenceModel->getValueAsString()));
                initComboBox(ui->comboTurbulenceSpecification,
                             pcConstraint->TurbulenceSpecification.getEnumVector(),
                             pcConstraint->TurbulenceSpecification.getValueAsString());
                ui->spinTurbulentIntensityValue->setValue(
                    pcConstraint->TurbulentIntensityValue.getValue());
                ui->spinTurbulentLengthValue->setValue(
                    pcConstraint->TurbulentLengthValue.getValue());
                updateTurbulenceUI();
            }
        }
        else {
            ui->tabTurbulenceBoundary->setEnabled(false);
        }
    }
    else {
        Base::Console().Warning(
            "No solver object inside FemAnalysis object, default to non-thermal, non-turbulence\n");
    }
    ui->tabWidget->setTabText(0, tr("Basic"));
    ui->tabWidget->setTabText(1, tr("Turbulence"));
    ui->tabWidget->setTabText(2, tr("Thermal"));
    ui->tabWidget->setCurrentIndex(0);
    ui->labelHelpText->setText(tr("select boundary type, faces and set value"));

    initComboBox(ui->comboBoundaryType,
                 pcConstraint->BoundaryType.getEnumVector(),
                 pcConstraint->BoundaryType.getValueAsString());
    updateBoundaryTypeUI();
    std::vector<std::string> subtypes = pcConstraint->Subtype.getEnumVector();
    initComboBox(ui->comboSubtype, subtypes, pcConstraint->Subtype.getValueAsString());
    updateSubtypeUI();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<std::string> dirStrings = pcConstraint->Direction.getSubValues();
    QString dir;
    if (!dirStrings.empty()) {
        dir = makeRefText(pcConstraint->Direction.getValue(), dirStrings.front());
    }

    // Fill data into dialog elements
    double f = pcConstraint->BoundaryValue.getValue();
    ui->spinBoundaryValue->setMinimum(FLOAT_MIN);  // previous set the min to ZERO is not flexible
    ui->spinBoundaryValue->setMaximum(FLOAT_MAX);
    ui->spinBoundaryValue->setValue(f);
    ui->listReferences->clear();
    for (std::size_t i = 0; i < Objects.size(); i++) {
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    }
    if (!Objects.empty()) {
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    }
    ui->lineDirection->setText(dir.isEmpty() ? tr("") : dir);
    ui->checkReverse->setVisible(true);  // it is still useful to swap direction of an edge

    ui->listReferences->blockSignals(false);
    ui->spinBoundaryValue->blockSignals(false);
    ui->buttonDirection->blockSignals(false);
    ui->checkReverse->blockSignals(false);

    updateUI();
}

const Fem::FemSolverObject* TaskFemConstraintFluidBoundary::getFemSolver() const
{
    return pcSolver;
}

void TaskFemConstraintFluidBoundary::updateBoundaryTypeUI()
{
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    std::string boundaryType = Base::Tools::toStdString(ui->comboBoundaryType->currentText());
    // std::string boundaryType = pcConstraint->BoundaryType.getValueAsString();

    // Update subtypes, any change here should be written back to FemConstraintFluidBoundary.cpp
    if (boundaryType == "wall") {
        ui->labelBoundaryValue->setText(QString::fromUtf8("velocity (m/s)"));
        ui->tabBasicBoundary->setEnabled(false);
        pcConstraint->Subtype.setEnums(WallSubtypes);
    }
    else if (boundaryType == "interface") {
        ui->labelBoundaryValue->setText(QString::fromUtf8("value not needed"));
        ui->tabBasicBoundary->setEnabled(false);
        pcConstraint->Subtype.setEnums(InterfaceSubtypes);
    }
    else if (boundaryType == "freestream") {
        ui->tabBasicBoundary->setEnabled(false);
        ui->labelBoundaryValue->setText(QString::fromUtf8("value not needed"));
        ui->tabBasicBoundary->setEnabled(false);
        pcConstraint->Subtype.setEnums(FreestreamSubtypes);
    }
    else if (boundaryType == "inlet") {
        ui->tabBasicBoundary->setEnabled(true);
        pcConstraint->Subtype.setEnums(InletSubtypes);
        ui->labelBoundaryValue->setText(QString::fromUtf8("Pressure [Pa]"));  // default to pressure
        pcConstraint->Reversed.setValue(true);  // inlet must point into volume
    }
    else if (boundaryType == "outlet") {
        ui->tabBasicBoundary->setEnabled(true);
        pcConstraint->Subtype.setEnums(OutletSubtypes);
        ui->labelBoundaryValue->setText(QString::fromUtf8("Pressure [Pa]"));
        pcConstraint->Reversed.setValue(false);  // outlet must point outward
    }
    else {
        Base::Console().Error("Error: Fluid boundary type `%s` is not defined\n",
                              boundaryType.c_str());
    }
    // std::string subtypeLabel = boundaryType + std::string(" type");
    // ui->labelSubtype->setText(QString::fromUtf8(subtypeLabel)); // too long to show in UI
    ui->tabWidget->setCurrentIndex(0);  // activate the basic pressure-momentum setting tab

    std::vector<std::string> subtypes = pcConstraint->Subtype.getEnumVector();
    initComboBox(ui->comboSubtype, subtypes, "default to the second subtype");
    updateSubtypeUI();
}


void TaskFemConstraintFluidBoundary::updateSubtypeUI()
{

    std::string boundaryType = Base::Tools::toStdString(ui->comboBoundaryType->currentText());
    std::string subtype = Base::Tools::toStdString(ui->comboSubtype->currentText());

    if (boundaryType == "inlet" || boundaryType == "outlet") {
        ui->tabBasicBoundary->setEnabled(true);
        if (subtype == "totalPressure" || subtype == "staticPressure") {
            ui->labelBoundaryValue->setText(QString::fromUtf8("pressure [Pa]"));
            ui->buttonDirection->setEnabled(false);
            ui->lineDirection->setEnabled(false);
        }
        else if (subtype == "uniformVelocity") {
            ui->labelBoundaryValue->setText(QString::fromUtf8("velocity [m/s]"));
            ui->buttonDirection->setEnabled(true);
            ui->lineDirection->setEnabled(true);
        }
        else if (subtype == "massFlowrate") {
            ui->labelBoundaryValue->setText(QString::fromUtf8("flowrate [kg/s]"));
            ui->buttonDirection->setEnabled(false);
            ui->lineDirection->setEnabled(false);
        }
        else if (subtype == "volumetricFlowRate") {
            ui->labelBoundaryValue->setText(QString::fromUtf8("flowrate [m3/s]"));
            ui->buttonDirection->setEnabled(false);
            ui->lineDirection->setEnabled(false);
        }
        else {
            ui->labelBoundaryValue->setText(QString::fromUtf8("unspecific"));
            ui->tabBasicBoundary->setEnabled(false);
        }
    }
    else if (boundaryType == "wall") {
        if (subtype == "moving") {
            ui->labelBoundaryValue->setText(QString::fromUtf8("moving speed (m/s)"));
            ui->tabBasicBoundary->setEnabled(true);
            ui->buttonDirection->setEnabled(false);  // moving speed must be parallel to wall
            ui->lineDirection->setEnabled(false);
        }
        else if (subtype == "slip") {
            ui->labelBoundaryValue->setText(QString::fromUtf8("not needed"));
            ui->tabBasicBoundary->setEnabled(false);
        }
        else if (subtype == "partialSlip") {
            ui->labelBoundaryValue->setText(QString::fromUtf8("slip ratio(0~1)"));
            ui->tabBasicBoundary->setEnabled(true);
            ui->buttonDirection->setEnabled(false);
            ui->lineDirection->setEnabled(false);
        }
        else {
            ui->labelBoundaryValue->setText(QString::fromUtf8("unspecific"));
            ui->tabBasicBoundary->setEnabled(false);
        }
    }
    else if (boundaryType == "interface") {
        ui->tabBasicBoundary->setEnabled(false);
        // show help text
        int iInterface = ui->comboSubtype->currentIndex();
        ui->labelHelpText->setText(tr(InterfaceSubtypeHelpTexts[iInterface]));
    }
    else if (boundaryType == "freestream") {
        ui->tabBasicBoundary->setEnabled(true);
    }
    else {
        Base::Console().Error("Fluid boundary type `%s` is not defined\n", boundaryType.c_str());
    }
}

void TaskFemConstraintFluidBoundary::updateTurbulenceUI()
{
    ui->labelHelpText->setText(
        tr(TurbulenceSpecificationHelpTexts[ui->comboTurbulenceSpecification->currentIndex()]));
    /// hide/disable UI only happened in constructor, update helptext and label text here
    std::string turbulenceSpec =
        Base::Tools::toStdString(ui->comboTurbulenceSpecification->currentText());
    ui->labelTurbulentIntensityValue->setText(tr("Intensity [0~1]"));
    if (turbulenceSpec == "intensity&DissipationRate") {
        ui->labelTurbulentLengthValue->setText(tr("Dissipation Rate [m2/s3]"));
    }
    else if (turbulenceSpec == "intensity&LengthScale") {
        ui->labelTurbulentLengthValue->setText(tr("Length Scale[m]"));
    }
    else if (turbulenceSpec == "intensity&ViscosityRatio") {
        ui->labelTurbulentLengthValue->setText(tr("Viscosity Ratio [1]"));
    }
    else if (turbulenceSpec == "intensity&HydraulicDiameter") {
        ui->labelTurbulentLengthValue->setText(tr("Hydraulic Diameter [m]"));
    }
    else {
        Base::Console().Error("turbulence Spec type `%s` is not defined\n", turbulenceSpec.c_str());
    }
}

void TaskFemConstraintFluidBoundary::updateThermalBoundaryUI()
{
    // Fem::ConstraintFluidBoundary* pcConstraint =
    // static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject()); std::string
    // thermalBoundaryType = pcConstraint->ThermalBoundaryType.getValueAsString();

    ui->labelHelpText->setText(
        tr(ThermalBoundaryHelpTexts[ui->comboThermalBoundaryType->currentIndex()]));
    // to hide/disable UI according to subtype
    std::string thermalBoundaryType =
        Base::Tools::toStdString(ui->comboThermalBoundaryType->currentText());
    ui->spinHTCoeffValue->setEnabled(false);
    ui->spinTemperatureValue->setEnabled(false);
    ui->spinHeatFluxValue->setEnabled(false);
    if (thermalBoundaryType == "zeroGradient" || thermalBoundaryType == "coupled") {
        return;
    }
    else if (thermalBoundaryType == "fixedValue") {
        ui->spinTemperatureValue->setEnabled(true);
    }
    else if (thermalBoundaryType == "fixedGradient") {
        ui->spinHeatFluxValue->setEnabled(true);
        ui->labelHeatFlux->setText(tr("Gradient [K/m]"));
    }
    else if (thermalBoundaryType == "mixed") {
        ui->spinTemperatureValue->setEnabled(true);
        ui->spinHeatFluxValue->setEnabled(true);
        ui->labelHeatFlux->setText(tr("Gradient [K/m]"));
    }
    else if (thermalBoundaryType == "heatFlux") {
        ui->spinHeatFluxValue->setEnabled(true);
        ui->labelHeatFlux->setText(tr("Flux [W/m2]"));
    }
    else if (thermalBoundaryType == "HTC") {
        ui->spinHTCoeffValue->setEnabled(true);
        ui->spinTemperatureValue->setEnabled(true);
    }
    else {
        Base::Console().Error("Thermal boundary type `%s` is not defined\n",
                              thermalBoundaryType.c_str());
    }
}

void TaskFemConstraintFluidBoundary::onBoundaryTypeChanged()
{
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    // temporarily change BoundaryType property, but command transaction should reset it back if you
    // 'reject' late
    pcConstraint->BoundaryType.setValue(ui->comboBoundaryType->currentIndex());
    updateBoundaryTypeUI();

    ConstraintView->updateData(&pcConstraint->BoundaryType);  // force a 3D redraw

    // update view provider once BoundaryType changed, updateData() may be just enough
    // FreeCAD.getDocument(pcConstraint->Document.getName()).recompute();
    bool ret = pcConstraint->recomputeFeature();
    if (!ret) {
        std::string boundaryType = ui->comboBoundaryType->currentText().toStdString();
        Base::Console().Error("Fluid boundary recomputationg failed for boundaryType `%s` \n",
                              boundaryType.c_str());
    }
}

void TaskFemConstraintFluidBoundary::onSubtypeChanged()
{
    updateSubtypeUI();  // todo: change color for different kind of subtype,
                        // Fem::ConstraintFluidBoundary::onChanged() and viewProvider
}

void TaskFemConstraintFluidBoundary::onBoundaryValueChanged(double)
{
    // left empty for future extension
}
void TaskFemConstraintFluidBoundary::onTurbulenceSpecificationChanged()
{
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    pcConstraint->TurbulenceSpecification.setValue(
        ui->comboTurbulenceSpecification->currentIndex());
    updateTurbulenceUI();
}

void TaskFemConstraintFluidBoundary::onThermalBoundaryTypeChanged()
{
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    pcConstraint->ThermalBoundaryType.setValue(ui->comboThermalBoundaryType->currentIndex());
    updateThermalBoundaryUI();
}

void TaskFemConstraintFluidBoundary::onReferenceDeleted()
{
    TaskFemConstraintFluidBoundary::removeFromSelection();  // On right-click face is automatically
                                                            // selected, so just remove
}

void TaskFemConstraintFluidBoundary::onButtonDirection(const bool pressed)
{
    // sets the normal vector of the currently selecteed planar face as direction

    Q_UNUSED(pressed);

    clearButtons(SelectionChangeModes::none);

    // get vector of selected objects of active document
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Empty selection"), tr("Select an edge or a face, please."));
        return;
    }
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());

    // we only handle the first selected object
    Gui::SelectionObject& selectionElement = selection.at(0);

    // we can only handle part objects
    if (!selectionElement.isObjectTypeOf(Part::Feature::getClassTypeId())) {
        QMessageBox::warning(this,
                             tr("Wrong selection"),
                             tr("Selected object is not a part object!"));
        return;
    }
    // get the names of the subobjects
    const std::vector<std::string>& subNames = selectionElement.getSubNames();

    if (subNames.size() != 1) {
        QMessageBox::warning(this,
                             tr("Wrong selection"),
                             tr("Only one planar face or edge can be selected!"));
        return;
    }

    // we are now sure we only have one object
    std::string subNamesElement = subNames[0];
    // vector for the direction
    std::vector<std::string> direction(1, subNamesElement);

    Part::Feature* feat = static_cast<Part::Feature*>(selectionElement.getObject());
    TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subNamesElement.c_str());

    if (subNamesElement.substr(0, 4) == "Face") {
        if (!Fem::Tools::isPlanar(TopoDS::Face(ref))) {
            QMessageBox::warning(this,
                                 tr("Wrong selection"),
                                 tr("Only planar faces can be picked for 3D"));
            return;
        }
    }
    else if (subNamesElement.substr(0, 4) == "Edge") {  // 2D or 3D can use edge as direction vector
        if (!Fem::Tools::isLinear(TopoDS::Edge(ref))) {
            QMessageBox::warning(this,
                                 tr("Wrong selection"),
                                 tr("Only planar edges can be picked for 2D"));
            return;
        }
    }
    else {
        QMessageBox::warning(this,
                             tr("Wrong selection"),
                             tr("Only faces for 3D part or edges for 2D can be picked"));
        return;
    }

    // update the direction
    pcConstraint->Direction.setValue(feat, direction);
    ui->lineDirection->setText(makeRefText(feat, subNamesElement));

    // Update UI
    updateUI();
}

void TaskFemConstraintFluidBoundary::onCheckReverse(const bool pressed)
{
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
}

std::string TaskFemConstraintFluidBoundary::getBoundaryType() const
{
    return Base::Tools::toStdString(ui->comboBoundaryType->currentText());
}

std::string TaskFemConstraintFluidBoundary::getSubtype() const
{
    return Base::Tools::toStdString(ui->comboSubtype->currentText());
}

double TaskFemConstraintFluidBoundary::getBoundaryValue() const
{
    return ui->spinBoundaryValue->value();
}


std::string TaskFemConstraintFluidBoundary::getTurbulenceModel() const
{
    if (pTurbulenceModel) {
        return pTurbulenceModel->getValueAsString();
    }
    else {
        return "laminar";
    }
}

std::string TaskFemConstraintFluidBoundary::getTurbulenceSpecification() const
{
    return Base::Tools::toStdString(ui->comboTurbulenceSpecification->currentText());
}

double TaskFemConstraintFluidBoundary::getTurbulentIntensityValue() const
{
    return ui->spinTurbulentIntensityValue->value();
}

double TaskFemConstraintFluidBoundary::getTurbulentLengthValue() const
{
    return ui->spinTurbulentLengthValue->value();
}

bool TaskFemConstraintFluidBoundary::getHeatTransferring() const
{
    if (pHeatTransferring) {
        return pHeatTransferring->getValue();
    }
    else {
        return false;
    }
}

std::string TaskFemConstraintFluidBoundary::getThermalBoundaryType() const
{
    return Base::Tools::toStdString(ui->comboThermalBoundaryType->currentText());
}

double TaskFemConstraintFluidBoundary::getTemperatureValue() const
{
    return ui->spinTemperatureValue->value();
}

double TaskFemConstraintFluidBoundary::getHeatFluxValue() const
{
    return ui->spinHeatFluxValue->value();
}

double TaskFemConstraintFluidBoundary::getHTCoeffValue() const
{
    return ui->spinHTCoeffValue->value();
}

const std::string TaskFemConstraintFluidBoundary::getReferences() const
{
    int rows = ui->listReferences->model()->rowCount();

    std::vector<std::string> items;
    for (int r = 0; r < rows; r++) {
        items.push_back(ui->listReferences->item(r)->text().toStdString());
    }
    return TaskFemConstraint::getReferences(items);
}

const std::string TaskFemConstraintFluidBoundary::getDirectionName() const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty()) {
        return "";
    }

    int pos = dir.find_last_of(":");
    return dir.substr(0, pos).c_str();
}

const std::string TaskFemConstraintFluidBoundary::getDirectionObject() const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty()) {
        return "";
    }

    int pos = dir.find_last_of(":");
    return dir.substr(pos + 1).c_str();
}

bool TaskFemConstraintFluidBoundary::getReverse() const
{
    return ui->checkReverse->isChecked();
}

TaskFemConstraintFluidBoundary::~TaskFemConstraintFluidBoundary() = default;

void TaskFemConstraintFluidBoundary::addToSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

    for (auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        App::DocumentObject* obj = it.getObject();
        for (const auto& subName : subNames) {  // for every selected sub element
            bool addMe = true;
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subName);
                 itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subName)) {  // for every sub element in selection that
                                              // matches one in old list
                if (obj
                    == Objects[std::distance(
                        SubElements.begin(),
                        itr)]) {  // if selected sub element's object equals the one in old list
                                  // then it was added before so don't add
                    addMe = false;
                }
            }
            // limit constraint such that only vertexes or faces or edges can be used depending on
            // what was selected first
            std::string searchStr;
            if (subName.find("Vertex") != std::string::npos) {
                searchStr = "Vertex";
            }
            else if (subName.find("Edge") != std::string::npos) {
                searchStr = "Edge";
            }
            else {
                searchStr = "Face";
            }

            for (const auto& SubElement : SubElements) {
                if (SubElement.find(searchStr) == std::string::npos) {
                    QString msg = tr("Only one type of selection (vertex, face or edge) per "
                                     "analysis feature allowed!");
                    QMessageBox::warning(this, tr("Selection error"), msg);
                    addMe = false;
                    break;
                }
            }
            if (addMe) {
                QSignalBlocker block(ui->listReferences);
                Objects.push_back(obj);
                SubElements.push_back(subName);
                ui->listReferences->addItem(makeRefText(obj, subName));
            }
        }
    }
    // Update UI
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintFluidBoundary::removeFromSelection()
{
    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx();  // gets vector of selected objects of active document
    if (selection.empty()) {
        QMessageBox::warning(this, tr("Selection error"), tr("Nothing selected!"));
        return;
    }
    Fem::ConstraintFluidBoundary* pcConstraint =
        static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<size_t> itemsToDel;
    for (const auto& it : selection) {  // for every selected object
        if (!it.isObjectTypeOf(Part::Feature::getClassTypeId())) {
            QMessageBox::warning(this, tr("Selection error"), tr("Selected object is not a part!"));
            return;
        }
        const std::vector<std::string>& subNames = it.getSubNames();
        const App::DocumentObject* obj = it.getObject();

        for (const auto& subName : subNames) {  // for every selected sub element
            for (std::vector<std::string>::iterator itr =
                     std::find(SubElements.begin(), SubElements.end(), subName);
                 itr != SubElements.end();
                 itr = std::find(++itr,
                                 SubElements.end(),
                                 subName)) {  // for every sub element in selection that
                                              // matches one in old list
                if (obj
                    == Objects[std::distance(
                        SubElements.begin(),
                        itr)]) {  // if selected sub element's object equals the one in old list
                                  // then it was added before so mark for deletion
                    itemsToDel.push_back(std::distance(SubElements.begin(), itr));
                }
            }
        }
    }
    std::sort(itemsToDel.begin(), itemsToDel.end());
    while (!itemsToDel.empty()) {
        Objects.erase(Objects.begin() + itemsToDel.back());
        SubElements.erase(SubElements.begin() + itemsToDel.back());
        itemsToDel.pop_back();
    }
    // Update UI
    {
        QSignalBlocker block(ui->listReferences);
        ui->listReferences->clear();
        for (size_t j = 0; j < Objects.size(); j++) {
            ui->listReferences->addItem(makeRefText(Objects[j], SubElements[j]));
        }
    }
    pcConstraint->References.setValues(Objects, SubElements);
    updateUI();
}

void TaskFemConstraintFluidBoundary::updateUI()
{
    if (ui->listReferences->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }
}

bool TaskFemConstraintFluidBoundary::event(QEvent* e)
{
    return TaskFemConstraint::KeyEvent(e);
}

void TaskFemConstraintFluidBoundary::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinBoundaryValue->blockSignals(true);
        // more ui widget? those UI are does not support tr yet!
        ui->retranslateUi(proxy);

        ui->spinBoundaryValue->blockSignals(false);
    }
}

void TaskFemConstraintFluidBoundary::clearButtons(const SelectionChangeModes notThis)
{
    if (notThis != SelectionChangeModes::refAdd) {
        ui->btnAdd->setChecked(false);
    }
    if (notThis != SelectionChangeModes::refRemove) {
        ui->btnRemove->setChecked(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintFluidBoundary::TaskDlgFemConstraintFluidBoundary(
    ViewProviderFemConstraintFluidBoundary* ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintFluidBoundary(ConstraintView);

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintFluidBoundary::open()
{
    // a transaction is already open when creating this panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Fluid boundary condition");
        Gui::Command::openCommand((const char*)msg.toUtf8());
    }
}

bool TaskDlgFemConstraintFluidBoundary::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintFluidBoundary* boundary =
        static_cast<const TaskFemConstraintFluidBoundary*>(parameter);

    // no need to backup pcConstraint object content, if rejected, content can be recovered by
    // transaction manager
    try {
        // Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Fluid boundary condition
        // changed"));
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.BoundaryType = '%s'",
                                name.c_str(),
                                boundary->getBoundaryType().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Subtype = '%s'",
                                name.c_str(),
                                boundary->getSubtype().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.BoundaryValue = %f",
                                name.c_str(),
                                boundary->getBoundaryValue());

        std::string dirname = boundary->getDirectionName().data();
        std::string dirobj = boundary->getDirectionObject().data();

        if (!dirname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(dirname));
            buf = buf.arg(QString::fromStdString(dirobj));
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.Direction = %s",
                                    name.c_str(),
                                    buf.toStdString().c_str());
        }
        else {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.Direction = None",
                                    name.c_str());
        }
        // Reverse control is done at BoundaryType selection, this UI is hidden from user
        // Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %s",
        // name.c_str(), boundary->getReverse() ? "True" : "False");

        std::string scale = boundary->getScale();  // OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,
                                "App.ActiveDocument.%s.Scale = %s",
                                name.c_str(),
                                scale.c_str());  // OvG: implement modified scale

        // solver specific setting, physical model selection
        const Fem::FemSolverObject* pcSolver = boundary->getFemSolver();

        if (pcSolver) {
            App::PropertyBool* pHeatTransferring = nullptr;
            App::PropertyEnumeration* pTurbulenceModel = nullptr;
            pHeatTransferring =
                static_cast<App::PropertyBool*>(pcSolver->getPropertyByName("HeatTransferring"));
            pTurbulenceModel = static_cast<App::PropertyEnumeration*>(
                pcSolver->getPropertyByName("TurbulenceModel"));

            if (pHeatTransferring && pHeatTransferring->getValue()) {
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.ThermalBoundaryType = '%s'",
                                        name.c_str(),
                                        boundary->getThermalBoundaryType().c_str());
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.TemperatureValue = %f",
                                        name.c_str(),
                                        boundary->getTemperatureValue());
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.HeatFluxValue = %f",
                                        name.c_str(),
                                        boundary->getHeatFluxValue());
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.HTCoeffValue = %f",
                                        name.c_str(),
                                        boundary->getHTCoeffValue());
            }
            if (pTurbulenceModel
                && std::string(pTurbulenceModel->getValueAsString())
                    != "laminar") {  // Invisic and DNS flow also does not need this
                // update turbulence and thermal boundary settings, only if those models are
                // activated
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.TurbulenceSpecification = '%s'",
                                        name.c_str(),
                                        boundary->getTurbulenceSpecification().c_str());
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.TurbulentIntensityValue = %f",
                                        name.c_str(),
                                        boundary->getTurbulentIntensityValue());
                Gui::Command::doCommand(Gui::Command::Doc,
                                        "App.ActiveDocument.%s.TurbulentLengthValue = %f",
                                        name.c_str(),
                                        boundary->getTurbulentLengthValue());
            }
        }
        else {
            Base::Console().Warning("FemSolverObject is not found in the FemAnalysis object, "
                                    "thermal and turbulence setting is not accepted\n");
        }
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return TaskDlgFemConstraint::accept();
}

bool TaskDlgFemConstraintFluidBoundary::reject()
{
    Gui::Command::abortCommand();  // recover properties content
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintFluidBoundary.cpp"
