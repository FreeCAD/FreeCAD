/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <TopExp_Explorer.hxx>
# include <QMessageBox>
# include <QButtonGroup>
#endif

#include "Tessellation.h"
#include "ui_Tessellation.h"
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Mesh/Gui/ViewProvider.h>
#include <Mod/Part/Gui/ViewProvider.h>

using namespace MeshPartGui;

/* TRANSLATOR MeshPartGui::Tessellation */

Tessellation::Tessellation(QWidget* parent)
  : QWidget(parent), ui(new Ui_Tessellation)
{
    ui->setupUi(this);

    buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(ui->radioButtonStandard, 0);
    buttonGroup->addButton(ui->radioButtonMefisto, 1);
    buttonGroup->addButton(ui->radioButtonNetgen, 2);
    connect(buttonGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(meshingMethod(int)));

    ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Mesh/Meshing/Standard");
    double value = ui->spinSurfaceDeviation->value().getValue();
    value = handle->GetFloat("LinearDeflection", value);
    double angle = ui->spinAngularDeviation->value().getValue();
    angle = handle->GetFloat("AngularDeflection", angle);
    bool relative = ui->relativeDeviation->isChecked();
    relative = handle->GetBool("RelativeLinearDeflection", relative);
    ui->relativeDeviation->setChecked(relative);

    ui->spinSurfaceDeviation->setMaximum(INT_MAX);
    ui->spinSurfaceDeviation->setValue(value);
    ui->spinAngularDeviation->setValue(angle);

    ui->spinMaximumEdgeLength->setRange(0, INT_MAX);

    // set the standard method
    ui->radioButtonStandard->setChecked(true);
    ui->comboFineness->setCurrentIndex(2);
    on_comboFineness_currentIndexChanged(2);

#if !defined (HAVE_MEFISTO)
    ui->radioButtonMefisto->setDisabled(true);
#else
    ui->radioButtonMefisto->setChecked(true);
#endif
#if !defined (HAVE_NETGEN)
    ui->radioButtonNetgen->setDisabled(true);
#else
    ui->radioButtonNetgen->setChecked(true);
#endif

    Gui::Command::doCommand(Gui::Command::Doc, "import Mesh");
    try {
        Gui::Command::doCommand(Gui::Command::Doc, "import MeshPart");
    }
    catch(...) {
        ui->radioButtonNetgen->setDisabled(true);
        ui->radioButtonMefisto->setDisabled(true);
        ui->radioButtonStandard->setChecked(true);
    }

    meshingMethod(buttonGroup->checkedId());
    findShapes();
}

Tessellation::~Tessellation()
{
}

void Tessellation::meshingMethod(int id)
{
    ui->stackedWidget->setCurrentIndex(id);
}

void Tessellation::on_comboFineness_currentIndexChanged(int index)
{
    if (index == 5) {
        ui->doubleGrading->setEnabled(true);
        ui->spinEdgeElements->setEnabled(true);
        ui->spinCurvatureElements->setEnabled(true);
    }
    else {
        ui->doubleGrading->setEnabled(false);
        ui->spinEdgeElements->setEnabled(false);
        ui->spinCurvatureElements->setEnabled(false);
    }

    switch (index) {
    case 0: // Very coarse
        ui->doubleGrading->setValue(0.7);
        ui->spinEdgeElements->setValue(0.3);
        ui->spinCurvatureElements->setValue(1.0);
        break;
    case 1: // Coarse
        ui->doubleGrading->setValue(0.5);
        ui->spinEdgeElements->setValue(0.5);
        ui->spinCurvatureElements->setValue(1.5);
        break;
    case 2: // Moderate
        ui->doubleGrading->setValue(0.3);
        ui->spinEdgeElements->setValue(1.0);
        ui->spinCurvatureElements->setValue(2.0);
        break;
    case 3: // Fine
        ui->doubleGrading->setValue(0.2);
        ui->spinEdgeElements->setValue(2.0);
        ui->spinCurvatureElements->setValue(3.0);
        break;
    case 4: // Very fine
        ui->doubleGrading->setValue(0.1);
        ui->spinEdgeElements->setValue(3.0);
        ui->spinCurvatureElements->setValue(5.0);
        break;
    default:
        break;
    }
}

void Tessellation::on_checkSecondOrder_toggled(bool on)
{
    if (on)
        ui->checkQuadDominated->setChecked(false);
}

void Tessellation::on_checkQuadDominated_toggled(bool on)
{
    if (on)
        ui->checkSecondOrder->setChecked(false);
}

void Tessellation::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int index = ui->comboFineness->currentIndex();
        ui->retranslateUi(this);
        ui->comboFineness->setCurrentIndex(index);
    }
    QWidget::changeEvent(e);
}

void Tessellation::findShapes()
{
    App::Document* activeDoc = App::GetApplication().getActiveDocument();
    if (!activeDoc) return;
    Gui::Document* activeGui = Gui::Application::Instance->getDocument(activeDoc);
    if (!activeGui) return;

    this->document = QString::fromLatin1(activeDoc->getName());
    std::vector<Part::Feature*> objs = activeDoc->getObjectsOfType<Part::Feature>();

    double edgeLen = 0;
    bool foundSelection = false;
    for (std::vector<Part::Feature*>::iterator it = objs.begin(); it!=objs.end(); ++it) {
        const TopoDS_Shape& shape = (*it)->Shape.getValue();
        if (shape.IsNull()) continue;
        bool hasfaces = false;
        TopExp_Explorer xp(shape,TopAbs_FACE);
        while (xp.More()) {
            hasfaces = true;
            break;
        }

        if (hasfaces) {
            Base::BoundBox3d bbox = (*it)->Shape.getBoundingBox();
            edgeLen = std::max<double>(edgeLen, bbox.LengthX());
            edgeLen = std::max<double>(edgeLen, bbox.LengthY());
            edgeLen = std::max<double>(edgeLen, bbox.LengthZ());
            QString label = QString::fromUtf8((*it)->Label.getValue());
            QString name = QString::fromLatin1((*it)->getNameInDocument());
            
            QTreeWidgetItem* child = new QTreeWidgetItem();
            child->setText(0, label);
            child->setToolTip(0, label);
            child->setData(0, Qt::UserRole, name);
            Gui::ViewProvider* vp = activeGui->getViewProvider(*it);
            if (vp) child->setIcon(0, vp->getIcon());
            ui->treeWidget->addTopLevelItem(child);
            if (Gui::Selection().isSelected(*it)) {
                child->setSelected(true);
                foundSelection = true;
            }
        }
    }

    ui->spinMaximumEdgeLength->setValue(edgeLen/10);
    if (foundSelection)
        ui->treeWidget->hide();
}

bool Tessellation::accept()
{
    if (ui->treeWidget->selectedItems().isEmpty()) {
        QMessageBox::critical(this, windowTitle(),
            tr("Select a shape for meshing, first."));
        return false;
    }

    App::Document* activeDoc = App::GetApplication().getDocument((const char*)this->document.toLatin1());
    if (!activeDoc) {
        QMessageBox::critical(this, windowTitle(),
            tr("No such document '%1'.").arg(this->document));
        return false;
    }

    try {
        QString shape, label;
        Gui::WaitCursor wc;

        int method = buttonGroup->checkedId();

        // Save parameters
        if (method == 0) {
            ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Mod/Mesh/Meshing/Standard");
            double value = ui->spinSurfaceDeviation->value().getValue();
            handle->SetFloat("LinearDeflection", value);
            double angle = ui->spinAngularDeviation->value().getValue();
            handle->SetFloat("AngularDeflection", angle);
            bool relative = ui->relativeDeviation->isChecked();
            handle->SetBool("RelativeLinearDeflection", relative);
        }

        activeDoc->openTransaction("Meshing");
        QList<QTreeWidgetItem *> items = ui->treeWidget->selectedItems();
        std::vector<Part::Feature*> shapes = Gui::Selection().getObjectsOfType<Part::Feature>();
        for (QList<QTreeWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
            shape = (*it)->data(0, Qt::UserRole).toString();
            label = (*it)->text(0);

            QString cmd;
            if (method == 0) { // Standard
                double devFace = ui->spinSurfaceDeviation->value().getValue();
                double devAngle = ui->spinAngularDeviation->value().getValue();
                devAngle = Base::toRadians<double>(devAngle);
                bool relative = ui->relativeDeviation->isChecked();
                QString param = QString::fromLatin1("Shape=__shape__, "
                                                    "LinearDeflection=%1, "
                                                    "AngularDeflection=%2, "
                                                    "Relative=%3")
                    .arg(devFace)
                    .arg(devAngle)
                    .arg(relative ? QString::fromLatin1("True") : QString::fromLatin1("False"));
                if (ui->meshShapeColors->isChecked())
                    param += QString::fromLatin1(",Segments=True");
                if (ui->groupsFaceColors->isChecked())
                    param += QString::fromLatin1(",GroupColors=__doc__.getObject(\"%1\").ViewObject.DiffuseColor")
                            .arg(shape);
                cmd = QString::fromLatin1(
                    "__doc__=FreeCAD.getDocument(\"%1\")\n"
                    "__mesh__=__doc__.addObject(\"Mesh::Feature\",\"Mesh\")\n"
                    "__part__=__doc__.getObject(\"%2\")\n"
                    "__shape__=__part__.Shape.copy(False)\n"
                    "__shape__.Placement=__part__.getGlobalPlacement()\n"
                    "__mesh__.Mesh=MeshPart.meshFromShape(%3)\n"
                    "__mesh__.Label=\"%4 (Meshed)\"\n"
                    "__mesh__.ViewObject.CreaseAngle=25.0\n"
                    "del __doc__, __mesh__, __part__, __shape__\n")
                    .arg(this->document)
                    .arg(shape)
                    .arg(param)
                    .arg(label);
            }
            else if (method == 1) { // Mefisto
                double maxEdge = ui->spinMaximumEdgeLength->value().getValue();
                if (!ui->spinMaximumEdgeLength->isEnabled())
                    maxEdge = 0;
                cmd = QString::fromLatin1(
                    "__doc__=FreeCAD.getDocument(\"%1\")\n"
                    "__mesh__=__doc__.addObject(\"Mesh::Feature\",\"Mesh\")\n"
                    "__part__=__doc__.getObject(\"%2\")\n"
                    "__shape__=__part__.Shape.copy(False)\n"
                    "__shape__.Placement=__part__.getGlobalPlacement()\n"
                    "__mesh__.Mesh=MeshPart.meshFromShape(Shape=__shape__,MaxLength=%3)\n"
                    "__mesh__.Label=\"%4 (Meshed)\"\n"
                    "__mesh__.ViewObject.CreaseAngle=25.0\n"
                    "del __doc__, __mesh__, __part__, __shape__\n")
                    .arg(this->document)
                    .arg(shape)
                    .arg(maxEdge)
                    .arg(label);
            }
            else if (method == 2) { // Netgen
                int fineness = ui->comboFineness->currentIndex();
                double growthRate = ui->doubleGrading->value();
                double nbSegPerEdge = ui->spinEdgeElements->value();
                double nbSegPerRadius = ui->spinCurvatureElements->value();
                bool secondOrder = ui->checkSecondOrder->isChecked();
                bool optimize = ui->checkOptimizeSurface->isChecked();
                bool allowquad = ui->checkQuadDominated->isChecked();
                if (fineness < 5) {
                    cmd = QString::fromLatin1(
                        "__doc__=FreeCAD.getDocument(\"%1\")\n"
                        "__mesh__=__doc__.addObject(\"Mesh::Feature\",\"Mesh\")\n"
                        "__part__=__doc__.getObject(\"%2\")\n"
                        "__shape__=__part__.Shape.copy(False)\n"
                        "__shape__.Placement=__part__.getGlobalPlacement()\n"
                        "__mesh__.Mesh=MeshPart.meshFromShape(Shape=__shape__,"
                        "Fineness=%3,SecondOrder=%4,Optimize=%5,AllowQuad=%6)\n"
                        "__mesh__.Label=\"%7 (Meshed)\"\n"
                        "__mesh__.ViewObject.CreaseAngle=25.0\n"
                        "del __doc__, __mesh__, __part__, __shape__\n")
                        .arg(this->document)
                        .arg(shape)
                        .arg(fineness)
                        .arg(secondOrder ? 1 : 0)
                        .arg(optimize ? 1 : 0)
                        .arg(allowquad ? 1 : 0)
                        .arg(label);
                }
                else {
                    cmd = QString::fromLatin1(
                        "__doc__=FreeCAD.getDocument(\"%1\")\n"
                        "__mesh__=__doc__.addObject(\"Mesh::Feature\",\"Mesh\")\n"
                        "__part__=__doc__.getObject(\"%2\")\n"
                        "__shape__=__part__.Shape.copy(False)\n"
                        "__shape__.Placement=__part__.getGlobalPlacement()\n"
                        "__mesh__.Mesh=MeshPart.meshFromShape(Shape=__shape__,"
                        "GrowthRate=%3,SegPerEdge=%4,SegPerRadius=%5,SecondOrder=%6,Optimize=%7,AllowQuad=%8)\n"
                        "__mesh__.Label=\"%9 (Meshed)\"\n"
                        "__mesh__.ViewObject.CreaseAngle=25.0\n"
                        "del __doc__, __mesh__, __part__, __shape__\n")
                        .arg(this->document)
                        .arg(shape)
                        .arg(growthRate)
                        .arg(nbSegPerEdge)
                        .arg(nbSegPerRadius)
                        .arg(secondOrder ? 1 : 0)
                        .arg(optimize ? 1 : 0)
                        .arg(allowquad ? 1 : 0)
                        .arg(label);
                }
            }
            Gui::Command::runCommand(Gui::Command::Doc, cmd.toUtf8());

            // if Standard mesher is used and face colors should be applied
            if (method == 0) { // Standard
                if (ui->meshShapeColors->isChecked()) {
                    Gui::ViewProvider* vpm = Gui::Application::Instance->getViewProvider
                            (activeDoc->getActiveObject());
                    Gui::ViewProvider* vpp = Gui::Application::Instance->getViewProvider
                            (activeDoc->getObject(shape.toLatin1()));
                    MeshGui::ViewProviderMesh* vpmesh = dynamic_cast<MeshGui::ViewProviderMesh*>(vpm);
                    PartGui::ViewProviderPart* vppart = dynamic_cast<PartGui::ViewProviderPart*>(vpp);
                    if (vpmesh && vppart) {
                        std::vector<App::Color> diff_col = vppart->DiffuseColor.getValues();
                        if (ui->groupsFaceColors->isChecked()) {
                            // unique colors
                            std::set<uint32_t> col_set;
                            for (auto it : diff_col)
                                col_set.insert(it.getPackedValue());
                            diff_col.clear();
                            for (auto it : col_set)
                                diff_col.push_back(App::Color(it));
                        }
                        vpmesh->highlightSegments(diff_col);
                    }
                }
            }
        }
        activeDoc->commitTransaction();
    }
    catch (const Base::Exception& e) {
        Base::Console().Error(e.what());
    }

    return true;
}

// ---------------------------------------

TaskTessellation::TaskTessellation()
{
    widget = new Tessellation();
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
        QPixmap()/*Gui::BitmapFactory().pixmap("MeshPart_Mesher")*/,
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskTessellation::~TaskTessellation()
{
    // automatically deleted in the sub-class
}

void TaskTessellation::open()
{
}

void TaskTessellation::clicked(int)
{
}

bool TaskTessellation::accept()
{
    return widget->accept();
}

bool TaskTessellation::reject()
{
    return true;
}

#include "moc_Tessellation.cpp"
