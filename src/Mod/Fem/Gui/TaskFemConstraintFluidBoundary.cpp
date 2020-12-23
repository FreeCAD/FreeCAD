/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
 *   Copyright (c) 2016 Qingfeng Xia <qingfeng.xia    iesensor.com>        *
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
# include <sstream>

# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <QAction>

# include <Precision.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
# include <Standard_PrimitiveTypes.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Geom_Plane.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <Geom_Line.hxx>
# include <gp_Lin.hxx>
#endif

#include "ui_TaskFemConstraintFluidBoundary.h"
#include "TaskFemConstraintFluidBoundary.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/Fem/App/FemConstraintFluidBoundary.h>
#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemTools.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Fem/App/FemAnalysis.h>
#include <Mod/Fem/App/FemSolverObject.h>
#include "ActiveAnalysisObserver.h"

#include <Base/Console.h>
#include <Base/Tools.h>

using namespace FemGui;
using namespace Gui;
using namespace Fem;

//also defined in FemConstrainFluidBoundary and foamcasebuilder/basicbuilder.py, please update simultaneously
//the second (index 1) is the default enum, as index 0 causes compiling error
//static const char* BoundaryTypes[] = {"inlet","wall","outlet","freestream", "interface", NULL};
static const char* WallSubtypes[] = {"unspecific", "fixed", "slip", "partialSlip", "moving", "rough", NULL};
static const char* InletSubtypes[] = {"unspecific","totalPressure","uniformVelocity","volumetricFlowRate","massFlowRate",NULL};
static const char* OutletSubtypes[] = {"unspecific","totalPressure","staticPressure","uniformVelocity", "outFlow", NULL};
static const char* InterfaceSubtypes[] = {"unspecific","symmetry","wedge","cyclic","empty", "coupled", NULL};
static const char* FreestreamSubtypes[] = {"unspecific", "freestream",NULL};

static const char* InterfaceSubtypeHelpTexts[] = {
    "invalid,select other valid interface subtype",
    "symmetry plane but not axis-sym axis line",
    "axis symmetric front and back surfaces",
    "periodic boundary in pair, treated as physical connected",
    "front and back for single layer 2D mesh, also axis-sym axis line",
    "exchange boundary vale with external program, need extra manual setup like file name", NULL};

// defined in file FemConstraintFluidBoundary:
// see Ansys fluet manual: Turbulence Specification method
//static const char* TurbulenceSpecifications[] = {"intensity&DissipationRate", "intensity&LengthScale","intensity&ViscosityRatio", "intensity&HydraulicDiameter",NULL};
//activate the heat transfer and radiation model in Solver object explorer
static const char* TurbulenceSpecificationHelpTexts[] = {
            "explicitly specific intensity k [SI unit] and dissipation rate epsilon [] / omega []",
            "intensity (0.05 ~ 0.15) and characteristic length scale of max eddy [m]",
            "intensity (0.05 ~ 0.15) and turbulent viscosity ratio",
            "for fully developed internal flow, Turbulence intensity (0-1.0) 0.05 typical", NULL};

//static const char* ThermalBoundaryTypes[] = {"fixedValue","zeroGradient", "fixedGradient", "mixed", "heatFlux", "HTC","coupled", NULL};
static const char* ThermalBoundaryHelpTexts[] = {"fixed Temperature [K]", "no heat transfer on boundary", "fixed value gradient [K/m]",
            "mixed fixedGradient and fixedValue", "fixed heat flux [W/m2]", "Heat transfer coeff [W/(M2)/K]", "conjugate heat transfer with solid", NULL};
// enable & disable quantityUI once valueType is selected

// internal function not declared in header file
void initComboBox(QComboBox* combo, const std::vector<std::string>& textItems, const std::string& sItem)
{
    combo->blockSignals(true);

    int iItem = 1; // the first one is "unspecific" (index 0)
    combo->clear();
    for (unsigned int it = 0; it < textItems.size(); it++)
    {
        combo->insertItem(it, Base::Tools::fromStdString(textItems[it]));
        if (sItem == textItems[it])
        {
            iItem = it;
            //Base::Console().Warning("Found the subtype and set the current index as %d for subtype %s ComboBox\n", it, sItem);
        }
    }
    combo->setCurrentIndex(iItem);
    combo->blockSignals(false);
}

/* TRANSLATOR FemGui::TaskFemConstraintFluidBoundary */
TaskFemConstraintFluidBoundary::TaskFemConstraintFluidBoundary(ViewProviderFemConstraintFluidBoundary *ConstraintView,QWidget *parent)
    : TaskFemConstraint(ConstraintView, parent, "fem-constraint-fluid-boundary")
    , dimension(-1)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskFemConstraintFluidBoundary();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    // Create a context menu for the listview of the references
    QAction* action = new QAction(tr("Delete"), ui->listReferences);
    action->connect(action, SIGNAL(triggered()),
                    this, SLOT(onReferenceDeleted()));
    ui->listReferences->addAction(action);
    ui->listReferences->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(ui->comboBoundaryType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onBoundaryTypeChanged(void)));
    connect(ui->comboSubtype, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onSubtypeChanged(void)));
    connect(ui->spinBoundaryValue, SIGNAL(valueChanged(double)),
            this, SLOT(onBoundaryValueChanged(double)));

    connect(ui->comboTurbulenceSpecification, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTurbulenceSpecificationChanged(void)));
    connect(ui->comboThermalBoundaryType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onThermalBoundaryTypeChanged(void)));

    connect(ui->buttonReference, SIGNAL(pressed()),
            this, SLOT(onButtonReference()));
    connect(ui->buttonDirection, SIGNAL(pressed()),
            this, SLOT(onButtonDirection()));
    connect(ui->checkReverse, SIGNAL(toggled(bool)),
            this, SLOT(onCheckReverse(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    ui->spinBoundaryValue->blockSignals(true);
    ui->listReferences->blockSignals(true);
    // boundaryType and subType combo signal is Temporarily prevented in initComboBox()
    ui->buttonReference->blockSignals(true);
    ui->buttonDirection->blockSignals(true);
    ui->checkReverse->blockSignals(true);

    // Get the feature data
    Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());

    Fem::FemAnalysis* pcAnalysis = NULL;
    if (FemGui::ActiveAnalysisObserver::instance()->hasActiveObject()) {
        pcAnalysis = FemGui::ActiveAnalysisObserver::instance()->getActiveObject();
    }
    else {
        App::Document* aDoc = pcConstraint->getDocument(); //  App::Application::GetApplication().getActiveDocument();
        std::vector<App::DocumentObject*> fem = aDoc->getObjectsOfType(Fem::FemAnalysis::getClassTypeId());
        if (fem.size() >=1) {
            pcAnalysis = static_cast<Fem::FemAnalysis*>(fem[0]);  // get the first
        }
    }

    Fem::FemMeshObject* pcMesh = NULL;
    if (pcAnalysis) {
        std::vector<App::DocumentObject*> fem = pcAnalysis->Group.getValues();
        for (std::vector<App::DocumentObject*>::iterator it = fem.begin(); it != fem.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(Fem::FemMeshObject::getClassTypeId()))
                pcMesh = static_cast<Fem::FemMeshObject*>(*it);
        }
    }
    else {
        Base::Console().Warning("FemAnalysis object is not activated or no FemAnalysis in the active document, mesh dimension is unknown\n");
        dimension = -1;  // unknown dimension of mesh
    }
    if (pcMesh != NULL) {
        App::Property* prop = pcMesh->getPropertyByName("Shape");  // PropertyLink
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
            App::PropertyLink* pcLink = static_cast<App::PropertyLink*>(prop);
            Part::Feature* pcPart = dynamic_cast<Part::Feature*>(pcLink->getValue());
            if (pcPart) {  // deduct dimension from part_obj.Shape.ShapeType
                const TopoDS_Shape & pShape = pcPart->Shape.getShape().getShape();
                const TopAbs_ShapeEnum shapeType = pShape.IsNull() ? TopAbs_SHAPE : pShape.ShapeType();
                if (shapeType == TopAbs_SOLID || shapeType ==TopAbs_COMPSOLID)  // COMPSOLID is solids connected by faces
                    dimension =3;
                else if (shapeType == TopAbs_FACE || shapeType == TopAbs_SHELL)
                    dimension =2;
                else if (shapeType == TopAbs_EDGE || shapeType == TopAbs_WIRE)
                    dimension =1;
                else
                    dimension =-1;  // Vertex (0D) can not make mesh, Compound type might contain any types
            }
            //Base::Console().Message("mesh dimension deducted from Part object of FemMeshObject is \n");
        }
    }

    pcSolver = NULL;  // this is an private object of type Fem::FemSolverObject*
    if (pcAnalysis) {
        std::vector<App::DocumentObject*> fem = pcAnalysis->Group.getValues();
        for (std::vector<App::DocumentObject*>::iterator it = fem.begin(); it != fem.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(Fem::FemSolverObject::getClassTypeId()))
                pcSolver = static_cast<Fem::FemSolverObject*>(*it);
        }
    }

    pHeatTransfering = NULL;
    pTurbulenceModel = NULL;
    if (pcSolver != NULL) {
        //if only it is CFD solver, otherwise exit by SIGSEGV error, detect getPropertyByName() !=  NULL
        if (pcSolver->getPropertyByName("HeatTransfering")) {
            pHeatTransfering = static_cast<App::PropertyBool*>(pcSolver->getPropertyByName("HeatTransfering"));
            if (pHeatTransfering->getValue()) {
                ui->tabThermalBoundary->setEnabled(true);
                initComboBox(ui->comboThermalBoundaryType, pcConstraint->ThermalBoundaryType.getEnumVector(),
                                pcConstraint->ThermalBoundaryType.getValueAsString());
                ui->spinHTCoeffValue->setValue(pcConstraint->HTCoeffValue.getValue());
                ui->spinHeatFluxValue->setValue(pcConstraint->HeatFluxValue.getValue());
                ui->spinTemperatureValue->setValue(pcConstraint->TemperatureValue.getValue());
                updateThermalBoundaryUI();
            }
            else {
                ui->tabThermalBoundary->setEnabled(false);  // could be hidden
                //Base::Console().Message("retrieve solver property HeatTransfering as false\n");
            }
        }
        else {
            ui->tabThermalBoundary->setEnabled(false);
        }
        if (pcSolver->getPropertyByName("TurbulenceModel")) {
            pTurbulenceModel = static_cast<App::PropertyEnumeration*>(pcSolver->getPropertyByName("TurbulenceModel"));
            if (pTurbulenceModel->getValueAsString() == std::string("laminar")){
                ui->tabTurbulenceBoundary->setEnabled(false);
            }
            else {
                ui->tabTurbulenceBoundary->setEnabled(true);
                ui->labelTurbulenceSpecification->setText(Base::Tools::fromStdString(
                            pTurbulenceModel->getValueAsString()));
                initComboBox(ui->comboTurbulenceSpecification, pcConstraint->TurbulenceSpecification.getEnumVector(),
                            pcConstraint->TurbulenceSpecification.getValueAsString());
                ui->spinTurbulentIntensityValue->setValue(pcConstraint->TurbulentIntensityValue.getValue());
                ui->spinTurbulentLengthValue->setValue(pcConstraint->TurbulentLengthValue.getValue());
                updateTurbulenceUI();
            }
        }
        else {
            ui->tabTurbulenceBoundary->setEnabled(false);
        }
    }
    else {
        Base::Console().Warning("No solver object inside FemAnalysis object, default to non-thermal, non-turbulence\n");
    }
    ui->tabWidget->setTabText(0, tr("Basic"));
    ui->tabWidget->setTabText(1, tr("Turbulence"));
    ui->tabWidget->setTabText(2, tr("Thermal"));
    ui->tabWidget->setCurrentIndex(0);
    ui->labelHelpText->setText(tr("select boundary type, faces and set value"));

    initComboBox(ui->comboBoundaryType, pcConstraint->BoundaryType.getEnumVector(),
                 pcConstraint->BoundaryType.getValueAsString());
    updateBoundaryTypeUI();
    std::vector<std::string> subtypes = pcConstraint->Subtype.getEnumVector();
    initComboBox(ui->comboSubtype, subtypes, pcConstraint->Subtype.getValueAsString());
    updateSubtypeUI();

    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    std::vector<std::string> dirStrings = pcConstraint->Direction.getSubValues();
    QString dir;
    if (!dirStrings.empty())
        dir = makeRefText(pcConstraint->Direction.getValue(), dirStrings.front());

    // Fill data into dialog elements
    double f = pcConstraint->BoundaryValue.getValue();
    ui->spinBoundaryValue->setMinimum(FLOAT_MIN);  // previous set the min to ZERO is not flexible
    ui->spinBoundaryValue->setMaximum(FLOAT_MAX);
    ui->spinBoundaryValue->setValue(f);
    ui->listReferences->clear();
    for (std::size_t i = 0; i < Objects.size(); i++)
        ui->listReferences->addItem(makeRefText(Objects[i], SubElements[i]));
    if (Objects.size() > 0)
        ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    ui->lineDirection->setText(dir.isEmpty() ? tr("") : dir);
    ui->checkReverse->setVisible(true); // it is still useful to swap direction of an edge

    ui->listReferences->blockSignals(false);
    ui->buttonReference->blockSignals(false);
    ui->spinBoundaryValue->blockSignals(false);
    ui->buttonDirection->blockSignals(false);
    ui->checkReverse->blockSignals(false);
    updateSelectionUI();
}

const Fem::FemSolverObject* TaskFemConstraintFluidBoundary::getFemSolver(void) const
{
    return pcSolver;
}

void TaskFemConstraintFluidBoundary::updateBoundaryTypeUI()
{
    Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    std::string boundaryType = Base::Tools::toStdString(ui->comboBoundaryType->currentText());
    //std::string boundaryType = pcConstraint->BoundaryType.getValueAsString();

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
        ui->labelBoundaryValue->setText(QString::fromUtf8("Pressure [Pa]")); // default to pressure
        pcConstraint->Reversed.setValue(true); // inlet must point into volume
    }
    else if (boundaryType == "outlet") {
        ui->tabBasicBoundary->setEnabled(true);
        pcConstraint->Subtype.setEnums(OutletSubtypes);
        ui->labelBoundaryValue->setText(QString::fromUtf8("Pressure [Pa]"));
        pcConstraint->Reversed.setValue(false); // outlet must point outward
    }
    else {
        Base::Console().Error("Error: Fluid boundary type `%s` is not defined\n", boundaryType.c_str());
    }
    //std::string subtypeLabel = boundaryType + std::string(" type");
    //ui->labelSubtype->setText(QString::fromUtf8(subtypeLabel)); // too long to show in UI
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
        if (subtype == "totalPressure" || subtype == "staticPressure"){
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
            ui->buttonDirection->setEnabled(false); // moving speed must be parallel to wall
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
        //show help text
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
    ui->labelHelpText->setText(tr(TurbulenceSpecificationHelpTexts[ui->comboTurbulenceSpecification->currentIndex()]));
    /// hide/disable UI only happened in constructor, update helptext and label text here
    std::string turbulenceSpec = Base::Tools::toStdString(ui->comboTurbulenceSpecification->currentText());
    ui->labelTurbulentIntensityValue->setText(tr("Intensity [0~1]"));
    if (turbulenceSpec == "intensity&DissipationRate"){
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
    //Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    //std::string thermalBoundaryType = pcConstraint->ThermalBoundaryType.getValueAsString();

    ui->labelHelpText->setText(tr(ThermalBoundaryHelpTexts[ui->comboThermalBoundaryType->currentIndex()]));
    //to hide/disable UI according to subtype
    std::string thermalBoundaryType = Base::Tools::toStdString(ui->comboThermalBoundaryType->currentText());
    ui->spinHTCoeffValue->setEnabled(false);
    ui->spinTemperatureValue->setEnabled(false);
    ui->spinHeatFluxValue->setEnabled(false);
    if (thermalBoundaryType == "zeroGradient" || thermalBoundaryType == "coupled"){
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
        Base::Console().Error("Thermal boundary type `%s` is not defined\n", thermalBoundaryType.c_str());
    }
}

void TaskFemConstraintFluidBoundary::updateSelectionUI()
{
    if (ui->listReferences->model()->rowCount() == 0) {
        // Go into reference selection mode if no reference has been selected yet
        onButtonReference(true);
        return;
    }

    /** not needed for fluid boundary, as it must be Face for 3D part,
     * Edge type boundary is needed for 2D CFD, but it is not supported yet
    std::string ref = ui->listReferences->item(0)->text().toStdString();
    int pos = ref.find_last_of(":");
    if (ref.substr(pos+1, 6) == "Vertex")
        ui->labelForce->setText(tr("Point load"));
    else if (ref.substr(pos+1, 4) == "Edge")
        ui->labelForce->setText(tr("Line load"));
    else if (ref.substr(pos+1, 4) == "Face")
        ui->labelForce->setText(tr("Area load"));
    */
}

void TaskFemConstraintFluidBoundary::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        // Don't allow selection in other document
        if (strcmp(msg.pDocName, ConstraintView->getObject()->getDocument()->getName()) != 0)
            return;

        if (!msg.pSubName || msg.pSubName[0] == '\0')
            return;
        std::string subName(msg.pSubName);

        if (selectionMode == selnone)
            return;

        std::vector<std::string> references(1,subName);
        Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
        App::DocumentObject* obj = ConstraintView->getObject()->getDocument()->getObject(msg.pObjectName);
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape ref = feat->Shape.getShape().getSubShape(subName.c_str());
        //* string conversion:  <Base/Tools.h> toStdString()/fromStdString()
        if (selectionMode == selref) {
            std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
            std::vector<std::string> SubElements = pcConstraint->References.getSubValues();

            // Ensure we don't have mixed reference types
            if (SubElements.size() > 0) {
                if (subName.substr(0,4) != SubElements.front().substr(0,4)) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Mixed shape types are not possible. Use a second constraint instead"));
                    return;
                }
            }
            else {
                if ((subName.substr(0,4) != "Face"  && dimension == 3)) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only faces can be picked for fluid boundary of 3D geometry"));
                    return;
                }
                if ((subName.substr(0,4) != "Edge"  && dimension == 2)) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only edges can be picked for fluid boundary of 2D geometry"));
                    return;
                }
            }

            // Avoid duplicates
            std::size_t pos = 0;
            for (; pos < Objects.size(); pos++) {
                if (obj == Objects[pos]) {
                    break;
                }
            }

            if (pos != Objects.size()) {
                if (subName == SubElements[pos]) {
                    return;
                }
            }

            // add the new reference
            Objects.push_back(obj);
            SubElements.push_back(subName);
            pcConstraint->References.setValues(Objects,SubElements);
            ui->listReferences->addItem(makeRefText(obj, subName));

            // Turn off reference selection mode
            onButtonReference(false);
        }
        else if (selectionMode == seldir) {  // select direction, can be Edge or Face(Face normal)
            if (subName.substr(0,4) == "Face" && dimension ==3) {
                if (!Fem::Tools::isPlanar(TopoDS::Face(ref))) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only planar faces can be picked for 3D"));
                    return;
                }
            }
            else if (subName.substr(0,4) == "Edge") {  // 2D or 3D can use edge as direction vector
                if (!Fem::Tools::isLinear(TopoDS::Edge(ref))) {
                    QMessageBox::warning(this, tr("Selection error"), tr("Only planar edges can be picked for 2D"));
                    return;
                }
            }
            else {
                QMessageBox::warning(this, tr("Selection error"), tr("Only faces for 3D part or edges for 2D can be picked"));
                return;
            }
            pcConstraint->Direction.setValue(obj, references);
            ui->lineDirection->setText(makeRefText(obj, subName));
            // Turn off direction selection mode
            onButtonDirection(false);
        }

        Gui::Selection().clearSelection();
        updateSelectionUI();
        // recompute (redrawing has been called by FemConstraint base class? )
        //bool ret = pcConstraint->recomputeFeature();  // not needed
    }
}

void TaskFemConstraintFluidBoundary::onBoundaryTypeChanged(void)
{
    Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    // temporarily change BoundaryType property, but command transaction should reset it back if you 'reject' late
    pcConstraint->BoundaryType.setValue(ui->comboBoundaryType->currentIndex());
    updateBoundaryTypeUI();

    ConstraintView->updateData(&pcConstraint->BoundaryType);  //force a 3D redraw

    // update view provider once BoundaryType changed, updateData() may be just enough
    //FreeCAD.getDocument(pcConstraint->Document.getName()).recompute();
    bool ret = pcConstraint->recomputeFeature();
    if (!ret) {
        std::string boundaryType = ui->comboBoundaryType->currentText().toStdString();
        Base::Console().Error("Fluid boundary recomputationg failed for boundaryType `%s` \n", boundaryType.c_str());
    }
}

void TaskFemConstraintFluidBoundary::onSubtypeChanged(void)
{
    updateSubtypeUI();  // todo: change color for different kind of subtype,  Fem::ConstraintFluidBoundary::onChanged() and viewProvider
}

void TaskFemConstraintFluidBoundary::onBoundaryValueChanged(double)
{
    //left empty for future extension
}
void TaskFemConstraintFluidBoundary::onTurbulenceSpecificationChanged(void)
{
    Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    pcConstraint->TurbulenceSpecification.setValue(ui->comboTurbulenceSpecification->currentIndex());
    updateTurbulenceUI();
}

void TaskFemConstraintFluidBoundary::onThermalBoundaryTypeChanged(void)
{
    Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    pcConstraint->ThermalBoundaryType.setValue(ui->comboThermalBoundaryType->currentIndex());
    updateThermalBoundaryUI();
}

void TaskFemConstraintFluidBoundary::onReferenceDeleted() {
    int row = ui->listReferences->currentIndex().row();
    TaskFemConstraint::onReferenceDeleted(row);
    ui->listReferences->model()->removeRow(row);
    ui->listReferences->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
}

void TaskFemConstraintFluidBoundary::onButtonDirection(const bool pressed) {
    if (pressed) {
        selectionMode = seldir;
    } else {
        selectionMode = selnone;
    }
    ui->buttonDirection->setChecked(pressed);
    Gui::Selection().clearSelection();
    /* minor bug:  once Direction property(edge link) is cleared in UI, arrow direction is not updated.
    Direction property can not be easily setup in C++, see example at the end of this file `accept()`
    redraw will only happen once taskpanel is closed,
    */
    //pcConstraint->Direction.setValue(pressed);
}

void TaskFemConstraintFluidBoundary::onCheckReverse(const bool pressed)
{
    Fem::ConstraintFluidBoundary* pcConstraint = static_cast<Fem::ConstraintFluidBoundary*>(ConstraintView->getObject());
    pcConstraint->Reversed.setValue(pressed);
}

std::string TaskFemConstraintFluidBoundary::getBoundaryType(void) const
{
    return Base::Tools::toStdString(ui->comboBoundaryType->currentText());
}

std::string TaskFemConstraintFluidBoundary::getSubtype(void) const
{
    return Base::Tools::toStdString(ui->comboSubtype->currentText());
}

double TaskFemConstraintFluidBoundary::getBoundaryValue(void) const
{
    return ui->spinBoundaryValue->value();
}


std::string TaskFemConstraintFluidBoundary::getTurbulenceModel(void) const
{
    if(pTurbulenceModel){
        return pTurbulenceModel->getValueAsString();
    }
    else{
        return "laminar";
    }
}

std::string TaskFemConstraintFluidBoundary::getTurbulenceSpecification(void) const
{
    return Base::Tools::toStdString(ui->comboTurbulenceSpecification->currentText());
}

double TaskFemConstraintFluidBoundary::getTurbulentIntensityValue(void) const
{
    return ui->spinTurbulentIntensityValue->value();
}

double TaskFemConstraintFluidBoundary::getTurbulentLengthValue(void) const
{
    return ui->spinTurbulentLengthValue->value();
}

bool TaskFemConstraintFluidBoundary::getHeatTransfering(void) const
{
    if(pHeatTransfering){
        return pHeatTransfering->getValue();
    }
    else{
        return false;
    }
}

std::string TaskFemConstraintFluidBoundary::getThermalBoundaryType(void) const
{
    return Base::Tools::toStdString(ui->comboThermalBoundaryType->currentText());
}

double TaskFemConstraintFluidBoundary::getTemperatureValue(void) const
{
    return ui->spinTemperatureValue->value();
}

double TaskFemConstraintFluidBoundary::getHeatFluxValue(void) const
{
    return ui->spinHeatFluxValue->value();
}

double TaskFemConstraintFluidBoundary::getHTCoeffValue(void) const
{
    return ui->spinHTCoeffValue->value();
}

const std::string TaskFemConstraintFluidBoundary::getReferences() const
{
    int rows = ui->listReferences->model()->rowCount();

    std::vector<std::string> items;
    for (int r = 0; r < rows; r++)
        items.push_back(ui->listReferences->item(r)->text().toStdString());
    return TaskFemConstraint::getReferences(items);
}

const std::string TaskFemConstraintFluidBoundary::getDirectionName(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(0, pos).c_str();
}

const std::string TaskFemConstraintFluidBoundary::getDirectionObject(void) const
{
    std::string dir = ui->lineDirection->text().toStdString();
    if (dir.empty())
        return "";

    int pos = dir.find_last_of(":");
    return dir.substr(pos+1).c_str();
}

bool TaskFemConstraintFluidBoundary::getReverse() const
{
    return ui->checkReverse->isChecked();
}

TaskFemConstraintFluidBoundary::~TaskFemConstraintFluidBoundary()
{
    delete ui;
}

void TaskFemConstraintFluidBoundary::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinBoundaryValue->blockSignals(true);
        //more ui widget? those UI are does not support tr yet!
        ui->retranslateUi(proxy);

        ui->spinBoundaryValue->blockSignals(false);
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFemConstraintFluidBoundary::TaskDlgFemConstraintFluidBoundary(ViewProviderFemConstraintFluidBoundary *ConstraintView)
{
    this->ConstraintView = ConstraintView;
    assert(ConstraintView);
    this->parameter = new TaskFemConstraintFluidBoundary(ConstraintView);;

    Content.push_back(parameter);
}

//==== calls from the TaskView ===============================================================

void TaskDlgFemConstraintFluidBoundary::open()
{
    // a transaction is already open when creating this panel
    if (!Gui::Command::hasPendingCommand()) {
        QString msg = QObject::tr("Constraint fluid boundary");
        Gui::Command::openCommand((const char*)msg.toUtf8());
    }
}

bool TaskDlgFemConstraintFluidBoundary::accept()
{
    std::string name = ConstraintView->getObject()->getNameInDocument();
    const TaskFemConstraintFluidBoundary* boundary = static_cast<const TaskFemConstraintFluidBoundary*>(parameter);

    // no need to backup pcConstraint object content, if rejected, content can be recovered by  transaction manager
    try {
        //Gui::Command::openCommand("Fluid boundary condition changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.BoundaryType = '%s'",
            name.c_str(), boundary->getBoundaryType().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Subtype = '%s'",
            name.c_str(), boundary->getSubtype().c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.BoundaryValue = %f",
            name.c_str(), boundary->getBoundaryValue());

        std::string dirname = boundary->getDirectionName().data();
        std::string dirobj = boundary->getDirectionObject().data();

        if (!dirname.empty()) {
            QString buf = QString::fromUtf8("(App.ActiveDocument.%1,[\"%2\"])");
            buf = buf.arg(QString::fromStdString(dirname));
            buf = buf.arg(QString::fromStdString(dirobj));
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = %s", name.c_str(), buf.toStdString().c_str());
        } else {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Direction = None", name.c_str());
        }
        //Reverse control is done at BoundaryType selection, this UI is hidden from user
        //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %s", name.c_str(), boundary->getReverse() ? "True" : "False");

        std::string scale = boundary->getScale();  //OvG: determine modified scale
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Scale = %s", name.c_str(), scale.c_str()); //OvG: implement modified scale

        // solver specific setting, physical model selection
        const Fem::FemSolverObject* pcSolver = boundary->getFemSolver();

        if (pcSolver) {
            App::PropertyBool* pHeatTransfering = NULL;
            App::PropertyEnumeration* pTurbulenceModel = NULL;
            pHeatTransfering = static_cast<App::PropertyBool*>(pcSolver->getPropertyByName("HeatTransfering"));
            pTurbulenceModel = static_cast<App::PropertyEnumeration*>(pcSolver->getPropertyByName("TurbulenceModel"));

            if (pHeatTransfering && pHeatTransfering->getValue()) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.ThermalBoundaryType = '%s'",name.c_str(), boundary->getThermalBoundaryType().c_str());
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.TemperatureValue = %f",name.c_str(), boundary->getTemperatureValue());
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.HeatFluxValue = %f",name.c_str(), boundary->getHeatFluxValue());
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.HTCoeffValue = %f",name.c_str(), boundary->getHTCoeffValue());
            }
            if (pTurbulenceModel && std::string(pTurbulenceModel->getValueAsString()) != "laminar") {  // Invisic and DNS flow also does not need this
                //update turbulence and thermal boundary settings, only if those models are activated
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.TurbulenceSpecification = '%s'",name.c_str(), boundary->getTurbulenceSpecification().c_str());
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.TurbulentIntensityValue = %f",name.c_str(), boundary->getTurbulentIntensityValue());
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.TurbulentLengthValue = %f",name.c_str(), boundary->getTurbulentLengthValue());
            }
        }
        else {
            Base::Console().Warning("FemSolverObject is not found in the FemAnalysis object, thermal and turbulence setting is not accepted\n");
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
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    Gui::Command::updateActive();

    return true;
}

#include "moc_TaskFemConstraintFluidBoundary.cpp"
