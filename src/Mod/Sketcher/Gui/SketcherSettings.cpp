/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QPainter>
# include <QPixmap>
# include <QMessageBox>
#endif

#include "SketcherSettings.h"
#include "ui_SketcherSettings.h"
#include "TaskSketcherGeneral.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <Gui/PrefWidgets.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Command.h>

using namespace SketcherGui;

/* TRANSLATOR SketcherGui::SketcherSettings */

SketcherSettings::SketcherSettings(QWidget* parent)
    : PreferencePage(parent), ui(new Ui_SketcherSettings)
{
    ui->setupUi(this);
    QGroupBox* groupBox = new QGroupBox(this);
    QGridLayout* gridLayout = new QGridLayout(groupBox);
    gridLayout->setSpacing(0);
    gridLayout->setMargin(0);
    form = new SketcherGeneralWidget(groupBox);
    gridLayout->addWidget(form, 0, 0, 1, 1);
    ui->gridLayout_3->addWidget(groupBox, 2, 0, 1, 1);

    // Don't need them at the moment
    ui->label_16->hide();
    ui->SketcherDatumWidth->hide();
    ui->label_12->hide();
    ui->DefaultSketcherVertexWidth->hide();
    ui->label_13->hide();
    ui->DefaultSketcherLineWidth->hide();

    QList < QPair<Qt::PenStyle, int> > styles;
    styles << qMakePair(Qt::SolidLine, 0xffff)
           << qMakePair(Qt::DashLine, 0x0f0f)
           << qMakePair(Qt::DotLine, 0xaaaa);
//           << qMakePair(Qt::DashDotLine, 0x????)
//           << qMakePair(Qt::DashDotDotLine, 0x????);
    ui->comboBox->setIconSize (QSize(80, 12));
    for (QList < QPair<Qt::PenStyle, int> >::iterator it = styles.begin(); it != styles.end(); ++it) {
        QPixmap px(ui->comboBox->iconSize());
        px.fill(Qt::transparent);
        QBrush brush(Qt::black);
        QPen pen(it->first);
        pen.setBrush(brush);
        pen.setWidth(2);

        QPainter painter(&px);
        painter.setPen(pen);
        double mid = ui->comboBox->iconSize().height() / 2.0;
        painter.drawLine(0, mid, ui->comboBox->iconSize().width(), mid);
        painter.end();

        ui->comboBox->addItem(QIcon(px), QString(), QVariant(it->second));
    }

    connect(ui->btnTVApply, SIGNAL(clicked(bool)), this, SLOT(onBtnTVApplyClicked(bool)));
}

/** 
 *  Destroys the object and frees any allocated resources
 */
SketcherSettings::~SketcherSettings()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void SketcherSettings::saveSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onSave();
    ui->SketchVertexColor->onSave();
    ui->EditedEdgeColor->onSave();
    ui->EditedVertexColor->onSave();
    ui->ConstructionColor->onSave();
    ui->ExternalColor->onSave();
    ui->FullyConstrainedColor->onSave();

    ui->ConstrainedColor->onSave();
    ui->NonDrivingConstraintColor->onSave();
    ui->DatumColor->onSave();

    ui->SketcherDatumWidth->onSave();
    ui->DefaultSketcherVertexWidth->onSave();
    ui->DefaultSketcherLineWidth->onSave();

    ui->CursorTextColor->onSave();

    // Sketch editing
    ui->EditSketcherFontSize->onSave();
    ui->SegmentsPerGeometry->onSave();
    ui->dialogOnDistanceConstraint->onSave();
    ui->continueMode->onSave();
    ui->checkBoxAdvancedSolverTaskBox->onSave();
    ui->checkBoxTVHideDependent->onSave();
    ui->checkBoxTVShowLinks->onSave();
    ui->checkBoxTVShowSupport->onSave();
    ui->checkBoxTVRestoreCamera->onSave();
    form->saveSettings();

    ParameterGrp::handle hViewGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int markerSize = ui->EditSketcherMarkerSize->itemData(ui->EditSketcherMarkerSize->currentIndex()).toInt();
    hViewGrp->SetInt("EditSketcherMarkerSize", markerSize);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
    QVariant data = ui->comboBox->itemData(ui->comboBox->currentIndex());
    int pattern = data.toInt();
    hGrp->SetInt("GridLinePattern", pattern);
}

void SketcherSettings::loadSettings()
{
    // Sketcher
    ui->SketchEdgeColor->onRestore();
    ui->SketchVertexColor->onRestore();
    ui->EditedEdgeColor->onRestore();
    ui->EditedVertexColor->onRestore();
    ui->ConstructionColor->onRestore();
    ui->ExternalColor->onRestore();
    ui->FullyConstrainedColor->onRestore();

    ui->ConstrainedColor->onRestore();
    ui->NonDrivingConstraintColor->onRestore();
    ui->DatumColor->onRestore();

    ui->SketcherDatumWidth->onRestore();
    ui->DefaultSketcherVertexWidth->onRestore();
    ui->DefaultSketcherLineWidth->onRestore();

    ui->CursorTextColor->onRestore();

    // Sketch editing
    ui->EditSketcherFontSize->onRestore();
    ui->SegmentsPerGeometry->onRestore();
    ui->dialogOnDistanceConstraint->onRestore();
    ui->continueMode->onRestore();
    ui->checkBoxAdvancedSolverTaskBox->onRestore();
    ui->checkBoxTVHideDependent->onRestore();
    ui->checkBoxTVShowLinks->onRestore();
    ui->checkBoxTVShowSupport->onRestore();
    ui->checkBoxTVRestoreCamera->onRestore();
    form->loadSettings();

    std::list<int> sizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_FILLED");
    for (std::list<int>::iterator it = sizes.begin(); it != sizes.end(); ++it)
        ui->EditSketcherMarkerSize->addItem(tr("%1 px").arg(*it), *it);
    ParameterGrp::handle hViewGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int markerSize = hViewGrp->GetInt("EditSketcherMarkerSize", 7);
    int markerIndex = ui->EditSketcherMarkerSize->findData(QVariant(markerSize));
    if (markerIndex < 0)
        markerIndex = 1;
    ui->EditSketcherMarkerSize->setCurrentIndex(markerIndex);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
    int pattern = hGrp->GetInt("GridLinePattern", 0x0f0f);
    int index = ui->comboBox->findData(QVariant(pattern));
    if (index <0) index = 1;
    ui->comboBox->setCurrentIndex(index);
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void SketcherSettings::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void SketcherSettings::onBtnTVApplyClicked(bool)
{
    QString errMsg;
    try{
        Gui::Command::doCommand(Gui::Command::Gui,
            "for name,doc in App.listDocuments().items():\n"
            "    for sketch in doc.findObjects('Sketcher::SketchObject'):\n"
            "        sketch.ViewObject.HideDependent = %s\n"
            "        sketch.ViewObject.ShowLinks = %s\n"
            "        sketch.ViewObject.ShowSupport = %s\n"
            "        sketch.ViewObject.RestoreCamera = %s\n",
            this->ui->checkBoxTVHideDependent->isChecked() ? "True": "False",
            this->ui->checkBoxTVShowLinks->isChecked()     ? "True": "False",
            this->ui->checkBoxTVShowSupport->isChecked()   ? "True": "False",
            this->ui->checkBoxTVRestoreCamera->isChecked() ? "True": "False");
    } catch (Base::PyException &e){
        Base::Console().Error("SketcherSettings::onBtnTVApplyClicked:\n");
        e.ReportException();
        errMsg = QString::fromLatin1(e.what());
    } catch (...) {
        errMsg = tr("Unexpected C++ exception");
    }
    if(errMsg.length()>0){
        QMessageBox::warning(this, tr("Sketcher"),errMsg);
    }
}

#include "moc_SketcherSettings.cpp"

