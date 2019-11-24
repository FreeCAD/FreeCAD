/***************************************************************************
 *   Copyright (c) 2015 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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
# include <boost/bind.hpp>
# include <QString>
#endif

#include "ui_TaskSketcherSolverAdvanced.h"
#include "TaskSketcherSolverAdvanced.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderSketch.h"

#define LM_EPS  1E-10
#define LM_EPS1 1E-80
#define LM_TAU  1E-3
#define DL_TOLG 1E-80
#define DL_TOLX 1E-80
#define DL_TOLF 1E-10
#define CONVERGENCE 1E-10
#define MAX_ITER 100
#define DEFAULT_SOLVER 2            // DL=2, LM=1, BFGS=0
#define DEFAULT_RSOLVER 2           // DL=2, LM=1, BFGS=0
#define DEFAULT_QRSOLVER 1          // DENSE=0, SPARSEQR=1
#define QR_PIVOT_THRESHOLD 1E-13    // under this value a Jacobian value is regarded as zero
#define DEFAULT_SOLVER_DEBUG 1      // None=0, Minimal=1, IterationLevel=2
#define MAX_ITER_MULTIPLIER false
#define DEFAULT_DOGLEG_GAUSS_STEP 0   // FullPivLU = 0, LeastNormFullPivLU = 1, LeastNormLdlt = 2

using namespace SketcherGui;
using namespace Gui::TaskView;

TaskSketcherSolverAdvanced::TaskSketcherSolverAdvanced(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Advanced solver control"),true, 0)
    , sketchView(sketchView)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskSketcherSolverAdvanced();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);

    ui->comboBoxDefaultSolver->onRestore();
    ui->comboBoxDogLegGaussStep->onRestore();
    ui->spinBoxMaxIter->onRestore();
    ui->checkBoxSketchSizeMultiplier->onRestore();
    ui->lineEditConvergence->onRestore();
    ui->comboBoxQRMethod->onRestore();
    ui->lineEditQRPivotThreshold->onRestore();
    ui->comboBoxRedundantDefaultSolver->onRestore();
    ui->spinBoxRedundantSolverMaxIterations->onRestore();
    ui->checkBoxRedundantSketchSizeMultiplier->onRestore();
    ui->lineEditRedundantConvergence->onRestore();
    ui->comboBoxDebugMode->onRestore();

    updateSketchObject();
}

TaskSketcherSolverAdvanced::~TaskSketcherSolverAdvanced()
{
    delete ui;
}

void TaskSketcherSolverAdvanced::updateDefaultMethodParameters(void)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/SolverAdvanced");

    int currentindex = ui->comboBoxDefaultSolver->currentIndex();
    int redundantcurrentindex = ui->comboBoxRedundantDefaultSolver->currentIndex();

    if(redundantcurrentindex == 2 || currentindex == 2)
        ui->comboBoxDogLegGaussStep->setEnabled(true);
    else
        ui->comboBoxDogLegGaussStep->setEnabled(false);

    switch(currentindex)
    {
        case 0: // BFGS
            ui->labelSolverParam1->setText(QString::fromLatin1(""));
            ui->labelSolverParam2->setText(QString::fromLatin1(""));
            ui->labelSolverParam3->setText(QString::fromLatin1(""));
            ui->lineEditSolverParam1->clear();
            ui->lineEditSolverParam2->clear();
            ui->lineEditSolverParam3->clear();
            ui->lineEditSolverParam1->setDisabled(true);
            ui->lineEditSolverParam2->setDisabled(true);
            ui->lineEditSolverParam3->setDisabled(true);
            break;
        case 1: // LM
        {
            ui->labelSolverParam1->setText(QString::fromLatin1("Eps"));
            ui->labelSolverParam2->setText(QString::fromLatin1("Eps1"));
            ui->labelSolverParam3->setText(QString::fromLatin1("Tau"));
            ui->lineEditSolverParam1->setEnabled(true);
            ui->lineEditSolverParam2->setEnabled(true);
            ui->lineEditSolverParam3->setEnabled(true);
            double eps = ::atof(hGrp->GetASCII("LM_eps",QString::number(LM_EPS).toUtf8()).c_str());
            double eps1 = ::atof(hGrp->GetASCII("LM_eps1",QString::number(LM_EPS1).toUtf8()).c_str());
            double tau = ::atof(hGrp->GetASCII("LM_tau",QString::number(LM_TAU).toUtf8()).c_str());
            ui->lineEditSolverParam1->setText(QString::number(eps).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditSolverParam2->setText(QString::number(eps1).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditSolverParam3->setText(QString::number(tau).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            sketchView->getSketchObject()->getSolvedSketch().setLM_eps(eps);
            sketchView->getSketchObject()->getSolvedSketch().setLM_eps1(eps1);
            sketchView->getSketchObject()->getSolvedSketch().setLM_tau(tau);
            break;
        }
        case 2: // DogLeg
        {
            ui->labelSolverParam1->setText(QString::fromLatin1("Tolg"));
            ui->labelSolverParam2->setText(QString::fromLatin1("Tolx"));
            ui->labelSolverParam3->setText(QString::fromLatin1("Tolf"));
            ui->lineEditSolverParam1->setEnabled(true);
            ui->lineEditSolverParam2->setEnabled(true);
            ui->lineEditSolverParam3->setEnabled(true);
            double tolg = ::atof(hGrp->GetASCII("DL_tolg",QString::number(DL_TOLG).toUtf8()).c_str());
            double tolx = ::atof(hGrp->GetASCII("DL_tolx",QString::number(DL_TOLX).toUtf8()).c_str());
            double tolf = ::atof(hGrp->GetASCII("DL_tolf",QString::number(DL_TOLF).toUtf8()).c_str());
            ui->lineEditSolverParam1->setText(QString::number(tolg).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditSolverParam2->setText(QString::number(tolx).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditSolverParam3->setText(QString::number(tolf).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolg(tolg);
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolf(tolf);
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolx(tolx);
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::updateRedundantMethodParameters(void)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/SolverAdvanced");

    int currentindex = ui->comboBoxDefaultSolver->currentIndex();
    int redundantcurrentindex = ui->comboBoxRedundantDefaultSolver->currentIndex();

    if(redundantcurrentindex == 2 || currentindex == 2)
        ui->comboBoxDogLegGaussStep->setEnabled(true);
    else
        ui->comboBoxDogLegGaussStep->setEnabled(false);

    switch(redundantcurrentindex)
    {
        case 0: // BFGS
            ui->labelRedundantSolverParam1->setText(QString::fromLatin1(""));
            ui->labelRedundantSolverParam2->setText(QString::fromLatin1(""));
            ui->labelRedundantSolverParam3->setText(QString::fromLatin1(""));
            ui->lineEditRedundantSolverParam1->clear();
            ui->lineEditRedundantSolverParam2->clear();
            ui->lineEditRedundantSolverParam3->clear();
            ui->lineEditRedundantSolverParam1->setDisabled(true);
            ui->lineEditRedundantSolverParam2->setDisabled(true);
            ui->lineEditRedundantSolverParam3->setDisabled(true);
            break;
        case 1: // LM
        {
            ui->labelRedundantSolverParam1->setText(QString::fromLatin1("R.Eps"));
            ui->labelRedundantSolverParam2->setText(QString::fromLatin1("R.Eps1"));
            ui->labelRedundantSolverParam3->setText(QString::fromLatin1("R.Tau"));
            ui->lineEditRedundantSolverParam1->setEnabled(true);
            ui->lineEditRedundantSolverParam2->setEnabled(true);
            ui->lineEditRedundantSolverParam3->setEnabled(true);
            double eps = ::atof(hGrp->GetASCII("Redundant_LM_eps",QString::number(LM_EPS).toUtf8()).c_str());
            double eps1 = ::atof(hGrp->GetASCII("Redundant_LM_eps1",QString::number(LM_EPS1).toUtf8()).c_str());
            double tau = ::atof(hGrp->GetASCII("Redundant_LM_tau",QString::number(LM_TAU).toUtf8()).c_str());
            ui->lineEditRedundantSolverParam1->setText(QString::number(eps).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditRedundantSolverParam2->setText(QString::number(eps1).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditRedundantSolverParam3->setText(QString::number(tau).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            sketchView->getSketchObject()->getSolvedSketch().setLM_epsRedundant(eps);
            sketchView->getSketchObject()->getSolvedSketch().setLM_eps1Redundant(eps1);
            sketchView->getSketchObject()->getSolvedSketch().setLM_tauRedundant(eps1);
            break;
        }
        case 2: // DogLeg
        {
            ui->labelRedundantSolverParam1->setText(QString::fromLatin1("R.Tolg"));
            ui->labelRedundantSolverParam2->setText(QString::fromLatin1("R.Tolx"));
            ui->labelRedundantSolverParam3->setText(QString::fromLatin1("R.Tolf"));
            ui->lineEditRedundantSolverParam1->setEnabled(true);
            ui->lineEditRedundantSolverParam2->setEnabled(true);
            ui->lineEditRedundantSolverParam3->setEnabled(true);
            double tolg = ::atof(hGrp->GetASCII("Redundant_DL_tolg",QString::number(DL_TOLG).toUtf8()).c_str());
            double tolx = ::atof(hGrp->GetASCII("Redundant_DL_tolx",QString::number(DL_TOLX).toUtf8()).c_str());
            double tolf = ::atof(hGrp->GetASCII("Redundant_DL_tolf",QString::number(DL_TOLF).toUtf8()).c_str());
            ui->lineEditRedundantSolverParam1->setText(QString::number(tolg).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditRedundantSolverParam2->setText(QString::number(tolx).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            ui->lineEditRedundantSolverParam3->setText(QString::number(tolf).remove(QString::fromLatin1("+").replace(QString::fromLatin1("e0"),QString::fromLatin1("E")).toUpper()));
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolgRedundant(tolg);
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolfRedundant(tolf);
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolxRedundant(tolx);
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::on_lineEditSolverParam1_editingFinished()
{
    QString text = ui->lineEditSolverParam1->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditSolverParam1->setText(sci.toUpper());

    switch(ui->comboBoxDefaultSolver->currentIndex())
    {
        case 1: // LM
        {
            sketchView->getSketchObject()->getSolvedSketch().setLM_eps(val);
            ui->lineEditSolverParam1->setEntryName("LM_eps");
            ui->lineEditSolverParam1->onSave();
            break;
        }
        case 2: // DogLeg
        {
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolg(val);
            ui->lineEditSolverParam1->setEntryName("DL_tolg");
            ui->lineEditSolverParam1->onSave();
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::on_lineEditRedundantSolverParam1_editingFinished()
{
    QString text = ui->lineEditRedundantSolverParam1->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditRedundantSolverParam1->setText(sci.toUpper());

    switch(ui->comboBoxDefaultSolver->currentIndex())
    {
        case 1: // LM
        {
            sketchView->getSketchObject()->getSolvedSketch().setLM_epsRedundant(val);
            ui->lineEditRedundantSolverParam1->setEntryName("Redundant_LM_eps");
            ui->lineEditRedundantSolverParam1->onSave();
            break;
        }
        case 2: // DogLeg
        {
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolgRedundant(val);
            ui->lineEditRedundantSolverParam1->setEntryName("Redundant_DL_tolg");
            ui->lineEditRedundantSolverParam1->onSave();
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::on_lineEditSolverParam2_editingFinished()
{
    QString text = ui->lineEditSolverParam2->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditSolverParam2->setText(sci.toUpper());

    switch(ui->comboBoxDefaultSolver->currentIndex())
    {
        case 1: // LM
        {
            sketchView->getSketchObject()->getSolvedSketch().setLM_eps1(val);
            ui->lineEditSolverParam2->setEntryName("LM_eps1");
            ui->lineEditSolverParam2->onSave();
            break;
        }
        case 2: // DogLeg
        {
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolx(val);
            ui->lineEditSolverParam2->setEntryName("DL_tolx");
            ui->lineEditSolverParam2->onSave();
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::on_lineEditRedundantSolverParam2_editingFinished()
{
    QString text = ui->lineEditRedundantSolverParam2->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditRedundantSolverParam2->setText(sci.toUpper());

    switch(ui->comboBoxDefaultSolver->currentIndex())
    {
        case 1: // LM
        {
            sketchView->getSketchObject()->getSolvedSketch().setLM_eps1Redundant(val);
            ui->lineEditRedundantSolverParam2->setEntryName("Redundant_LM_eps1");
            ui->lineEditRedundantSolverParam2->onSave();
            break;
        }
        case 2: // DogLeg
        {
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolxRedundant(val);
            ui->lineEditRedundantSolverParam2->setEntryName("Redundant_DL_tolx");
            ui->lineEditRedundantSolverParam2->onSave();
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::on_lineEditSolverParam3_editingFinished()
{
    QString text = ui->lineEditSolverParam3->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditSolverParam3->setText(sci.toUpper());

    switch(ui->comboBoxDefaultSolver->currentIndex())
    {
        case 1: // LM
        {
            sketchView->getSketchObject()->getSolvedSketch().setLM_tau(val);
            ui->lineEditSolverParam3->setEntryName("LM_tau");
            ui->lineEditSolverParam3->onSave();
            break;
        }
        case 2: // DogLeg
        {
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolf(val);
            ui->lineEditSolverParam3->setEntryName("DL_tolf");
            ui->lineEditSolverParam3->onSave();
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::on_lineEditRedundantSolverParam3_editingFinished()
{
    QString text = ui->lineEditRedundantSolverParam3->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditRedundantSolverParam3->setText(sci.toUpper());

    switch(ui->comboBoxDefaultSolver->currentIndex())
    {
        case 1: // LM
        {
            sketchView->getSketchObject()->getSolvedSketch().setLM_tauRedundant(val);
            ui->lineEditRedundantSolverParam3->setEntryName("Redundant_LM_tau");
            ui->lineEditRedundantSolverParam3->onSave();
            break;
        }
        case 2: // DogLeg
        {
            sketchView->getSketchObject()->getSolvedSketch().setDL_tolfRedundant(val);
            ui->lineEditRedundantSolverParam3->setEntryName("Redundant_DL_tolf");
            ui->lineEditRedundantSolverParam3->onSave();
            break;
        }
    }
}

void TaskSketcherSolverAdvanced::on_comboBoxDefaultSolver_currentIndexChanged(int index)
{
    ui->comboBoxDefaultSolver->onSave();
    sketchView->getSketchObject()->getSolvedSketch().defaultSolver=(GCS::Algorithm) index;
    updateDefaultMethodParameters();
}

void TaskSketcherSolverAdvanced::on_comboBoxDogLegGaussStep_currentIndexChanged(int index)
{
    ui->comboBoxDogLegGaussStep->onSave();
    sketchView->getSketchObject()->getSolvedSketch().setDogLegGaussStep((GCS::DogLegGaussStep) index);
    updateDefaultMethodParameters();
}

void TaskSketcherSolverAdvanced::on_spinBoxMaxIter_valueChanged(int i)
{
    ui->spinBoxMaxIter->onSave();
    sketchView->getSketchObject()->getSolvedSketch().setMaxIter(i);
}

void TaskSketcherSolverAdvanced::on_checkBoxSketchSizeMultiplier_stateChanged(int state)
{
    if(state==Qt::Checked) {
        ui->checkBoxSketchSizeMultiplier->onSave();
        sketchView->getSketchObject()->getSolvedSketch().setSketchSizeMultiplier(true);
    }
    else if (state==Qt::Unchecked) {
        ui->checkBoxSketchSizeMultiplier->onSave();
        sketchView->getSketchObject()->getSolvedSketch().setSketchSizeMultiplier(false);
    }
}

void TaskSketcherSolverAdvanced::on_lineEditQRPivotThreshold_editingFinished()
{
    QString text = ui->lineEditQRPivotThreshold->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditQRPivotThreshold->setText(sci.toUpper());

    ui->lineEditQRPivotThreshold->onSave();

    sketchView->getSketchObject()->getSolvedSketch().setQRPivotThreshold(val);
}

void TaskSketcherSolverAdvanced::on_lineEditConvergence_editingFinished()
{
    QString text = ui->lineEditConvergence->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditConvergence->setText(sci.toUpper());

    ui->lineEditConvergence->onSave();

    sketchView->getSketchObject()->getSolvedSketch().setConvergence(val);
}

void TaskSketcherSolverAdvanced::on_lineEditRedundantConvergence_editingFinished()
{
    QString text = ui->lineEditRedundantConvergence->text();
    double val = text.toDouble();
    QString sci = QString::number(val);
    sci.remove(QString::fromLatin1("+"));
    sci.replace(QString::fromLatin1("e0"),QString::fromLatin1("E"));
    ui->lineEditRedundantConvergence->setText(sci.toUpper());

    ui->lineEditRedundantConvergence->onSave();

    sketchView->getSketchObject()->getSolvedSketch().setConvergenceRedundant(val);
}

void TaskSketcherSolverAdvanced::on_comboBoxQRMethod_currentIndexChanged(int index)
{
    sketchView->getSketchObject()->getSolvedSketch().setQRAlgorithm((GCS::QRAlgorithm) index);
    ui->comboBoxQRMethod->onSave();
}

void TaskSketcherSolverAdvanced::on_comboBoxRedundantDefaultSolver_currentIndexChanged(int index)
{
    ui->comboBoxRedundantDefaultSolver->onSave();
    sketchView->getSketchObject()->getSolvedSketch().defaultSolverRedundant=(GCS::Algorithm) index;
    updateRedundantMethodParameters();
}

void TaskSketcherSolverAdvanced::on_spinBoxRedundantSolverMaxIterations_valueChanged(int i)
{
    ui->spinBoxRedundantSolverMaxIterations->onSave();
    sketchView->getSketchObject()->getSolvedSketch().setMaxIterRedundant(i);
}

void TaskSketcherSolverAdvanced::on_checkBoxRedundantSketchSizeMultiplier_stateChanged(int state)
{
    if(state==Qt::Checked) {
        ui->checkBoxRedundantSketchSizeMultiplier->onSave();
        sketchView->getSketchObject()->getSolvedSketch().setSketchSizeMultiplierRedundant(true);
    }
    else if (state==Qt::Unchecked) {
        ui->checkBoxRedundantSketchSizeMultiplier->onSave();
        sketchView->getSketchObject()->getSolvedSketch().setSketchSizeMultiplierRedundant(true);
    }
}

void TaskSketcherSolverAdvanced::on_comboBoxDebugMode_currentIndexChanged(int index)
{
    ui->comboBoxDebugMode->onSave();
    sketchView->getSketchObject()->getSolvedSketch().setDebugMode((GCS::DebugMode) index);
}

void TaskSketcherSolverAdvanced::on_pushButtonSolve_clicked(bool checked/* = false*/)
{
    Q_UNUSED(checked);
    sketchView->getSketchObject()->solve();
}

void TaskSketcherSolverAdvanced::on_pushButtonDefaults_clicked(bool checked/* = false*/)
{
    Q_UNUSED(checked);
    // Algorithm params for default solvers
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/SolverAdvanced");
    hGrp->SetASCII("LM_eps",QString::number(LM_EPS).toUtf8());
    hGrp->SetASCII("LM_eps1",QString::number(LM_EPS1).toUtf8());
    hGrp->SetASCII("LM_tau",QString::number(LM_TAU).toUtf8());
    hGrp->SetASCII("DL_tolg",QString::number(DL_TOLG).toUtf8());
    hGrp->SetASCII("DL_tolx",QString::number(DL_TOLX).toUtf8());
    hGrp->SetASCII("DL_tolf",QString::number(DL_TOLF).toUtf8());
    hGrp->SetASCII("Redundant_LM_eps",QString::number(LM_EPS).toUtf8());
    hGrp->SetASCII("Redundant_LM_eps1",QString::number(LM_EPS1).toUtf8());
    hGrp->SetASCII("Redundant_LM_tau",QString::number(LM_TAU).toUtf8());
    hGrp->SetASCII("Redundant_DL_tolg",QString::number(DL_TOLG).toUtf8());
    hGrp->SetASCII("Redundant_DL_tolx",QString::number(DL_TOLX).toUtf8());
    hGrp->SetASCII("Redundant_DL_tolf",QString::number(DL_TOLF).toUtf8());
    // Set other settings
    hGrp->SetInt("DefaultSolver",DEFAULT_SOLVER);
    hGrp->SetInt("DogLegGaussStep",DEFAULT_DOGLEG_GAUSS_STEP);

    hGrp->SetInt("RedundantDefaultSolver",DEFAULT_RSOLVER);
    hGrp->SetInt("MaxIter",MAX_ITER);
    hGrp->SetInt("RedundantSolverMaxIterations",MAX_ITER);
    hGrp->SetBool("SketchSizeMultiplier",MAX_ITER_MULTIPLIER);
    hGrp->SetBool("RedundantSketchSizeMultiplier",MAX_ITER_MULTIPLIER);
    hGrp->SetASCII("Convergence",QString::number(CONVERGENCE).toUtf8());
    hGrp->SetASCII("RedundantConvergence",QString::number(CONVERGENCE).toUtf8());
    hGrp->SetInt("QRMethod",DEFAULT_QRSOLVER);
    hGrp->SetASCII("QRPivotThreshold",QString::number(QR_PIVOT_THRESHOLD).toUtf8());
    hGrp->SetInt("DebugMode",DEFAULT_SOLVER_DEBUG);

    ui->comboBoxDefaultSolver->onRestore();
    ui->comboBoxDogLegGaussStep->onRestore();
    ui->spinBoxMaxIter->onRestore();
    ui->checkBoxSketchSizeMultiplier->onRestore();
    ui->lineEditConvergence->onRestore();
    ui->comboBoxQRMethod->onRestore();
    ui->lineEditQRPivotThreshold->onRestore();
    ui->comboBoxRedundantDefaultSolver->onRestore();
    ui->spinBoxRedundantSolverMaxIterations->onRestore();
    ui->checkBoxRedundantSketchSizeMultiplier->onRestore();
    ui->lineEditRedundantConvergence->onRestore();
    ui->comboBoxDebugMode->onRestore();

    updateSketchObject();
}

void TaskSketcherSolverAdvanced::updateSketchObject(void)
{
    sketchView->getSketchObject()->getSolvedSketch().setDebugMode((GCS::DebugMode) ui->comboBoxDebugMode->currentIndex());
    sketchView->getSketchObject()->getSolvedSketch().setSketchSizeMultiplierRedundant(ui->checkBoxRedundantSketchSizeMultiplier->isChecked());
    sketchView->getSketchObject()->getSolvedSketch().setMaxIterRedundant(ui->spinBoxRedundantSolverMaxIterations->value());
    sketchView->getSketchObject()->getSolvedSketch().defaultSolverRedundant=(GCS::Algorithm) ui->comboBoxRedundantDefaultSolver->currentIndex();
    sketchView->getSketchObject()->getSolvedSketch().setQRAlgorithm((GCS::QRAlgorithm) ui->comboBoxQRMethod->currentIndex());
    sketchView->getSketchObject()->getSolvedSketch().setQRPivotThreshold(ui->lineEditQRPivotThreshold->text().toDouble());
    sketchView->getSketchObject()->getSolvedSketch().setConvergenceRedundant(ui->lineEditRedundantConvergence->text().toDouble());
    sketchView->getSketchObject()->getSolvedSketch().setConvergence(ui->lineEditConvergence->text().toDouble());
    sketchView->getSketchObject()->getSolvedSketch().setSketchSizeMultiplier(ui->checkBoxSketchSizeMultiplier->isChecked());
    sketchView->getSketchObject()->getSolvedSketch().setMaxIter(ui->spinBoxMaxIter->value());
    sketchView->getSketchObject()->getSolvedSketch().defaultSolver=(GCS::Algorithm) ui->comboBoxDefaultSolver->currentIndex();
    sketchView->getSketchObject()->getSolvedSketch().setDogLegGaussStep((GCS::DogLegGaussStep) ui->comboBoxDogLegGaussStep->currentIndex());

    updateDefaultMethodParameters();
    updateRedundantMethodParameters();
}

#include "moc_TaskSketcherSolverAdvanced.cpp"
