/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QDialog>
# include <QDoubleSpinBox>
# include <QVBoxLayout>
# include <QMessageBox>
# include <QPointer>
#endif

#include "SegmentationBestFit.h"
#include "ui_SegmentationBestFit.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <Gui/SelectionObject.h>
#include <Mod/Mesh/App/Core/Approximation.h>
#include <Mod/Mesh/App/Core/Segmentation.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshFeature.h>

using namespace MeshGui;

namespace MeshGui {
class PlaneFitParameter : public FitParameter
{
public:
    PlaneFitParameter() {}
    virtual ~PlaneFitParameter() {}
    virtual std::vector<float> getParameter(FitParameter::Points pts) const {
        std::vector<float> values;
        MeshCore::PlaneFit fit;
        fit.AddPoints(pts);
        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base = fit.GetBase();
            Base::Vector3f axis = fit.GetNormal();
            values.push_back(base.x);
            values.push_back(base.y);
            values.push_back(base.z);
            values.push_back(axis.x);
            values.push_back(axis.y);
            values.push_back(axis.z);
        }
        return values;
    }
};

class CylinderFitParameter : public FitParameter
{
public:
    CylinderFitParameter() {}
    virtual ~CylinderFitParameter() {}
    virtual std::vector<float> getParameter(FitParameter::Points pts) const {
        std::vector<float> values;
        MeshCore::CylinderFit fit;
        fit.AddPoints(pts);
        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base = fit.GetBase();
            Base::Vector3f axis = fit.GetAxis();
            float radius = fit.GetRadius();
            values.push_back(base.x);
            values.push_back(base.y);
            values.push_back(base.z);
            values.push_back(axis.x);
            values.push_back(axis.y);
            values.push_back(axis.z);
            values.push_back(radius);
        }
        return values;
    }
};

class SphereFitParameter : public FitParameter
{
public:
    SphereFitParameter() {}
    virtual ~SphereFitParameter() {}
    virtual std::vector<float> getParameter(FitParameter::Points pts) const {
        std::vector<float> values;
        MeshCore::SphereFit fit;
        fit.AddPoints(pts);
        if (fit.Fit() < FLOAT_MAX) {
            Base::Vector3f base = fit.GetCenter();
            float radius = fit.GetRadius();
            values.push_back(base.x);
            values.push_back(base.y);
            values.push_back(base.z);
            values.push_back(radius);
        }
        return values;
    }
};
}

ParametersDialog::ParametersDialog(std::vector<float>& val, FitParameter* fitPar,
                                   ParameterList par, Mesh::Feature* mesh,
                                   QWidget* parent)
    : QDialog(parent)
    , values(val)
    , fitParameter(fitPar)
    , parameter(par)
    , myMesh(mesh)
{
    this->setWindowTitle(tr("Surface fit"));

    QGridLayout *gridLayout;
    gridLayout = new QGridLayout(this);

    QGroupBox *groupBox;
    groupBox = new QGroupBox(this);
    groupBox->setTitle(tr("Parameters"));
    gridLayout->addWidget(groupBox, 0, 0, 1, 1);

    QGroupBox *selectBox;
    selectBox = new QGroupBox(this);
    selectBox->setTitle(tr("Selection"));
    gridLayout->addWidget(selectBox, 1, 0, 1, 1);

    QVBoxLayout *selectLayout;
    selectLayout = new QVBoxLayout(selectBox);

    QPushButton *regionButton;
    regionButton = new QPushButton(this);
    regionButton->setText(tr("Region"));
    regionButton->setObjectName(QString::fromLatin1("region"));
    selectLayout->addWidget(regionButton);

    QPushButton *singleButton;
    singleButton = new QPushButton(this);
    singleButton->setText(tr("Triangle"));
    singleButton->setObjectName(QString::fromLatin1("single"));
    selectLayout->addWidget(singleButton);

    QPushButton *clearButton;
    clearButton = new QPushButton(this);
    clearButton->setText(tr("Clear"));
    clearButton->setObjectName(QString::fromLatin1("clear"));
    selectLayout->addWidget(clearButton);

    QPushButton *computeButton;
    computeButton = new QPushButton(this);
    computeButton->setText(tr("Compute"));
    computeButton->setObjectName(QString::fromLatin1("compute"));
    gridLayout->addWidget(computeButton, 2, 0, 1, 1);

    QDialogButtonBox *buttonBox;
    buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    gridLayout->addWidget(buttonBox, 3, 0, 1, 1);

    int index = 0;
    QGridLayout *layout;
    layout = new QGridLayout(groupBox);
    for (auto it : parameter) {
        QLabel* label = new QLabel(groupBox);
        label->setText(it.first);
        layout->addWidget(label, index, 0, 1, 1);

        QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox(groupBox);
        doubleSpinBox->setObjectName(it.first);
        doubleSpinBox->setRange(-INT_MAX, INT_MAX);
        doubleSpinBox->setValue(it.second);
        layout->addWidget(doubleSpinBox, index, 1, 1, 1);
        spinBoxes.push_back(doubleSpinBox);
        ++index;
    }

    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QMetaObject::connectSlotsByName(this);

    Gui::SelectionObject obj(mesh);
    std::vector<Gui::SelectionObject> sel;
    sel.push_back(obj);
    meshSel.setObjects(sel);
    meshSel.setCheckOnlyPointToUserTriangles(true);
    meshSel.setCheckOnlyVisibleTriangles(true);
}

ParametersDialog::~ParametersDialog()
{
    meshSel.clearSelection();
    delete fitParameter;
}

void ParametersDialog::on_region_clicked()
{
    meshSel.startSelection();
}

void ParametersDialog::on_single_clicked()
{
    meshSel.selectTriangle();
}

void ParametersDialog::on_clear_clicked()
{
    meshSel.clearSelection();
}

void ParametersDialog::on_compute_clicked()
{
    const Mesh::MeshObject& kernel = myMesh->Mesh.getValue();
    if (kernel.hasSelectedFacets()) {
        std::vector<unsigned long> facets, points;
        kernel.getFacetsFromSelection(facets);
        points = kernel.getPointsFromFacets(facets);
        MeshCore::MeshPointArray coords = kernel.getKernel().GetPoints(points);

        // Copy points into right format
        FitParameter::Points fitpts;
        fitpts.insert(fitpts.end(), coords.begin(), coords.end());
        coords.clear();

        values = fitParameter->getParameter(fitpts);
        if (values.size() == spinBoxes.size()) {
            for (std::size_t i=0; i<values.size(); i++) {
                spinBoxes[i]->setValue(values[i]);
            }
        }
        meshSel.stopSelection();
        meshSel.clearSelection();
    }
    else {
        QMessageBox::warning(this, tr("No selection"), tr("Before fitting the surface select an area."));
    }
}

void ParametersDialog::accept()
{
    std::vector<float> v;
    for (auto it : spinBoxes)
        v.push_back(it->value());
    values = v;
    QDialog::accept();
}

void ParametersDialog::reject()
{
    values.clear();
    QDialog::reject();
}

// ----------------------------------------------------------------------------

/* TRANSLATOR MeshGui::SegmentationBestFit */

SegmentationBestFit::SegmentationBestFit(Mesh::Feature* mesh, QWidget* parent, Qt::WindowFlags fl)
  : QWidget(parent, fl), myMesh(mesh)
{
    ui = new Ui_SegmentationBestFit;
    ui->setupUi(this);
    ui->numPln->setRange(1, INT_MAX);
    ui->numPln->setValue(100);
    ui->numCyl->setRange(1, INT_MAX);
    ui->numCyl->setValue(100);
    ui->numSph->setRange(1, INT_MAX);
    ui->numSph->setValue(100);

    Gui::SelectionObject obj(myMesh);
    std::vector<Gui::SelectionObject> sel;
    sel.push_back(obj);
    meshSel.setObjects(sel);
}

SegmentationBestFit::~SegmentationBestFit()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void SegmentationBestFit::on_planeParameters_clicked()
{
    ParameterList list;
    std::vector<float> p = planeParameter;
    p.resize(6);
    QString base = tr("Base");
    QString axis = tr("Normal");
    QString x = QString::fromLatin1(" x");
    QString y = QString::fromLatin1(" y");
    QString z = QString::fromLatin1(" z");
    list.push_back(std::make_pair(base + x, p[0]));
    list.push_back(std::make_pair(base + y, p[1]));
    list.push_back(std::make_pair(base + z, p[2]));
    list.push_back(std::make_pair(axis + x, p[3]));
    list.push_back(std::make_pair(axis + y, p[4]));
    list.push_back(std::make_pair(axis + z, p[5]));

    static QPointer<QDialog> dialog = 0;
    if (!dialog)
        dialog = new ParametersDialog(planeParameter,
                                      new PlaneFitParameter,
                                      list, myMesh, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void SegmentationBestFit::on_cylinderParameters_clicked()
{
    ParameterList list;
    std::vector<float> p = cylinderParameter;
    p.resize(7);
    QString base = tr("Base");
    QString axis = tr("Axis");
    QString radius = tr("Radius");
    QString x = QString::fromLatin1(" x");
    QString y = QString::fromLatin1(" y");
    QString z = QString::fromLatin1(" z");
    list.push_back(std::make_pair(base + x, p[0]));
    list.push_back(std::make_pair(base + y, p[1]));
    list.push_back(std::make_pair(base + z, p[2]));
    list.push_back(std::make_pair(axis + x, p[3]));
    list.push_back(std::make_pair(axis + y, p[4]));
    list.push_back(std::make_pair(axis + z, p[5]));
    list.push_back(std::make_pair(radius,   p[6]));

    static QPointer<QDialog> dialog = 0;
    if (!dialog)
        dialog = new ParametersDialog(cylinderParameter,
                                      new CylinderFitParameter,
                                      list, myMesh, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void SegmentationBestFit::on_sphereParameters_clicked()
{
    ParameterList list;
    std::vector<float> p = sphereParameter;
    p.resize(4);
    QString base = tr("Center");
    QString radius = tr("Radius");
    QString x = QString::fromLatin1(" x");
    QString y = QString::fromLatin1(" y");
    QString z = QString::fromLatin1(" z");
    list.push_back(std::make_pair(base + x, p[0]));
    list.push_back(std::make_pair(base + y, p[1]));
    list.push_back(std::make_pair(base + z, p[2]));
    list.push_back(std::make_pair(radius,   p[3]));

    static QPointer<QDialog> dialog = 0;
    if (!dialog)
        dialog = new ParametersDialog(sphereParameter,
                                      new SphereFitParameter,
                                      list, myMesh, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void SegmentationBestFit::accept()
{
    const Mesh::MeshObject* mesh = myMesh->Mesh.getValuePtr();
    const MeshCore::MeshKernel& kernel = mesh->getKernel();

    MeshCore::MeshSegmentAlgorithm finder(kernel);

    std::vector<MeshCore::MeshSurfaceSegment*> segm;
    if (ui->groupBoxCyl->isChecked()) {
        MeshCore::AbstractSurfaceFit* fitter;
        if (cylinderParameter.size() == 7) {
            std::vector<float>& p = cylinderParameter;
            fitter = new MeshCore::CylinderSurfaceFit(
                Base::Vector3f(p[0],p[1],p[2]),
                Base::Vector3f(p[3],p[4],p[5]),
                p[6]);
        }
        else {
            fitter = new MeshCore::CylinderSurfaceFit;
        }
        segm.push_back(new MeshCore::MeshDistanceGenericSurfaceFitSegment
            (fitter, kernel, ui->numCyl->value(), ui->tolCyl->value()));
    }
    if (ui->groupBoxSph->isChecked()) {
        MeshCore::AbstractSurfaceFit* fitter;
        if (sphereParameter.size() == 4) {
            std::vector<float>& p = sphereParameter;
            fitter = new MeshCore::SphereSurfaceFit(
                Base::Vector3f(p[0],p[1],p[2]),
                p[3]);
        }
        else {
            fitter = new MeshCore::SphereSurfaceFit;
        }
        segm.push_back(new MeshCore::MeshDistanceGenericSurfaceFitSegment
            (fitter, kernel, ui->numSph->value(), ui->tolSph->value()));
    }
    if (ui->groupBoxPln->isChecked()) {
        MeshCore::AbstractSurfaceFit* fitter;
        if (planeParameter.size() == 6) {
            std::vector<float>& p = planeParameter;
            fitter = new MeshCore::PlaneSurfaceFit(
                Base::Vector3f(p[0],p[1],p[2]),
                Base::Vector3f(p[3],p[4],p[5]));
        }
        else {
            fitter = new MeshCore::PlaneSurfaceFit;
        }
        segm.push_back(new MeshCore::MeshDistanceGenericSurfaceFitSegment
            (fitter, kernel, ui->numPln->value(), ui->tolPln->value()));
    }
    finder.FindSegments(segm);

    App::Document* document = App::GetApplication().getActiveDocument();
    document->openTransaction("Segmentation");

    std::string internalname = "Segments_";
    internalname += myMesh->getNameInDocument();
    App::DocumentObjectGroup* group = static_cast<App::DocumentObjectGroup*>(document->addObject
        ("App::DocumentObjectGroup", internalname.c_str()));
    std::string labelname = "Segments ";
    labelname += myMesh->Label.getValue();
    group->Label.setValue(labelname);
    for (std::vector<MeshCore::MeshSurfaceSegment*>::iterator it = segm.begin(); it != segm.end(); ++it) {
        const std::vector<MeshCore::MeshSegment>& data = (*it)->GetSegments();
        for (std::vector<MeshCore::MeshSegment>::const_iterator jt = data.begin(); jt != data.end(); ++jt) {
            Mesh::MeshObject* segment = mesh->meshFromSegment(*jt);
            Mesh::Feature* feaSegm = static_cast<Mesh::Feature*>(group->addObject("Mesh::Feature", "Segment"));
            Mesh::MeshObject* feaMesh = feaSegm->Mesh.startEditing();
            feaMesh->swap(*segment);
            feaSegm->Mesh.finishEditing();
            delete segment;

            std::stringstream label;
            label << feaSegm->Label.getValue() << " (" << (*it)->GetType() << ")";
            feaSegm->Label.setValue(label.str());
        }
        delete (*it);
    }
    document->commitTransaction();
}

void SegmentationBestFit::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

// ---------------------------------------

/* TRANSLATOR MeshGui::TaskSegmentationBestFit */

TaskSegmentationBestFit::TaskSegmentationBestFit(Mesh::Feature* mesh)
{
    widget = new SegmentationBestFit(mesh);
    taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskSegmentationBestFit::~TaskSegmentationBestFit()
{
    // automatically deleted in the sub-class
}

bool TaskSegmentationBestFit::accept()
{
    widget->accept();
    return true;
}

#include "moc_SegmentationBestFit.cpp"
