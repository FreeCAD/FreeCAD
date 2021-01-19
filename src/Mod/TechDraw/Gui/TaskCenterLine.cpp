/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com                 *
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
#include <cmath>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#endif // #ifndef _PreComp_

#include <QButtonGroup>
#include <QStatusBar>
#include <QGraphicsScene>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include <Mod/TechDraw/Gui/ui_TaskCenterLine.h>

#include "DrawGuiStd.h"
#include "PreferencesGui.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"
#include "Rez.h"

#include "TaskCenterLine.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for edit
TaskCenterLine::TaskCenterLine(TechDraw::DrawViewPart* partFeat,
                               TechDraw::DrawPage* page,
                               std::string edgeName,
                               bool editMode) :
    ui(new Ui_TaskCenterLine),
    m_partFeat(partFeat),
    m_basePage(page),
    m_createMode(false),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_edgeName(edgeName),
    m_extendBy(0.0),
    m_clIdx(0),
    m_type(0),          //0 - Face, 1 - 2 Lines, 2 - 2 points
    m_mode(0),           //0 - vertical, 1 - horizontal, 2 - aligned
    m_editMode(editMode)
{
//    Base::Console().Message("TCL::TCL() - edit mode\n");
    ui->setupUi(this);

    m_geomIndex = DrawUtil::getIndexFromName(m_edgeName);
    const std::vector<TechDraw::BaseGeom  *> &geoms = partFeat->getEdgeGeometry();
    BaseGeom* bg = geoms.at(m_geomIndex);
    std::string tag = bg->getCosmeticTag();
    m_cl = partFeat->getCenterLine(tag);
    if (m_cl == nullptr) {         //checked by CommandAnnotate.  Should never happen.
        Base::Console().Message("TCL::TCL() - no centerline found\n");
    }
    else {
        m_type = m_cl->m_type;
        m_mode = m_cl->m_mode;
    }

    setUiEdit();
}

//ctor for creation
TaskCenterLine::TaskCenterLine(TechDraw::DrawViewPart* partFeat,
                               TechDraw::DrawPage* page,
                               std::vector<std::string> subNames,
                               bool editMode) :
    ui(new Ui_TaskCenterLine),
    m_partFeat(partFeat),
    m_basePage(page),
    m_createMode(true),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_subNames(subNames),
    m_extendBy(0.0),
    m_geomIndex(0),
    m_cl(nullptr),
    m_clIdx(0),
    m_type(0),          //0 - Face, 1 - 2 Lines, 2 - 2 points
    m_mode(0),           //0 - vertical, 1 - horizontal, 2 - aligned
    m_editMode(editMode)
{
//    Base::Console().Message("TCL::TCL() - create mode\n");
    if ( (m_basePage == nullptr) ||
         (m_partFeat == nullptr) )  {
        //should be caught in CMD caller
        Base::Console().Error("TaskCenterLine - bad parameters.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);
    std::string check = subNames.front();
    std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(check);
    if (geomType == "Face") {
        m_type = 0;
    } else if (geomType == "Edge") {
        m_type = 1;
    } else if (geomType == "Vertex") {
        m_type = 2;
    } else {
        Base::Console().Error("TaskCenterLine - unknown geometry type: %s.  Can not proceed.\n", geomType.c_str());
        return;
    }

    setUiPrimary();
}

TaskCenterLine::~TaskCenterLine()
{
}

void TaskCenterLine::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskCenterLine::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskCenterLine::setUiPrimary()
{
    setWindowTitle(QObject::tr("Create Center Line"));

    if (m_partFeat != nullptr) {
        std::string baseName = m_partFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        for (auto& s: m_subNames) {
            QString listItem = Base::Tools::fromStdString(s);
            ui->lstSubList->addItem(listItem);
        }
    }
    ui->cpLineColor->setColor(getCenterColor());
    ui->dsbWeight->setValue(getCenterWidth());
    ui->cboxStyle->setCurrentIndex(getCenterStyle() - 1);

    ui->qsbVertShift->setUnit(Base::Unit::Length);
    ui->qsbHorizShift->setUnit(Base::Unit::Length);
    Base::Quantity qVal;
    qVal.setUnit(Base::Unit::Length);
    qVal.setValue(getExtendBy());
    ui->qsbExtend->setValue(qVal);

    Base::Quantity qAngle;
    qAngle.setUnit(Base::Unit::Angle);
    ui->qsbRotate->setValue(qAngle);
    int precision = Base::UnitsApi::getDecimals();
    ui->qsbRotate->setDecimals(precision);

    if (m_type == 0) // if face, then aligned is not possible
        ui->rbAligned->setEnabled(false);
    else
        ui->rbAligned->setEnabled(true);

    if (m_type == 1) // only if line, feature is enabled
        ui->cbFlip->setEnabled(true);
    else
        ui->cbFlip->setEnabled(false);
}

void TaskCenterLine::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Center Line"));
    if (m_partFeat != nullptr) {
        std::string baseName = m_partFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        QString listItem = Base::Tools::fromStdString(m_edgeName);
        ui->lstSubList->addItem(listItem);
    }
    ui->cpLineColor->setColor(m_cl->m_format.m_color.asValue<QColor>());
    connect(ui->cpLineColor, SIGNAL(changed()), this, SLOT(onColorChanged()));
    ui->dsbWeight->setValue(m_cl->m_format.m_weight);
    connect(ui->dsbWeight, SIGNAL(valueChanged(double)), this, SLOT(onWeightChanged()));
    ui->cboxStyle->setCurrentIndex(m_cl->m_format.m_style - 1);
    connect(ui->cboxStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(onStyleChanged()));

    ui->rbVertical->setChecked(false);
    ui->rbHorizontal->setChecked(false);
    ui->rbAligned->setChecked(false);
    if (m_cl->m_mode == 0)
        ui->rbVertical->setChecked(true);
    else if (m_cl->m_mode == 1)
        ui->rbHorizontal->setChecked(true);
    else if (m_cl->m_mode ==2)
        ui->rbAligned->setChecked(true);
    if (m_cl->m_type == 0) // if face, then aligned is not possible
        ui->rbAligned->setEnabled(false);
    else
        ui->rbAligned->setEnabled(true);

    Base::Quantity qVal;
    qVal.setUnit(Base::Unit::Length);
    qVal.setValue(m_cl->m_vShift);
    ui->qsbVertShift->setValue(qVal);
    connect(ui->qsbVertShift, SIGNAL(valueChanged(double)), this, SLOT(onShiftVertChanged()));
    qVal.setValue(m_cl->m_hShift);
    ui->qsbHorizShift->setValue(qVal);
    connect(ui->qsbHorizShift, SIGNAL(valueChanged(double)), this, SLOT(onShiftHorizChanged()));
    qVal.setValue(m_cl->m_extendBy);
    ui->qsbExtend->setValue(qVal);
    connect(ui->qsbExtend, SIGNAL(valueChanged(double)), this, SLOT(onExtendChanged()));

    Base::Quantity qAngle;
    qAngle.setUnit(Base::Unit::Angle);
    ui->qsbRotate->setValue(qAngle);
    int precision = Base::UnitsApi::getDecimals();
    ui->qsbRotate->setDecimals(precision);
    ui->qsbRotate->setValue(m_cl->m_rotate);
    connect(ui->qsbRotate, SIGNAL(valueChanged(double)), this, SLOT(onRotationChanged()));

    if (m_cl->m_flip2Line)
        ui->cbFlip->setChecked(true);
    else
        ui->cbFlip->setChecked(false);

    if (m_cl->m_type == 1) // only if line, feature is enabled
        ui->cbFlip->setEnabled(true);
    else
        ui->cbFlip->setEnabled(false);
    connect(ui->cbFlip, SIGNAL(toggled(bool)), this, SLOT(onFlipChanged()));

    // connect the Orientation radio group box
    connect(ui->bgOrientation, SIGNAL(buttonClicked(int)), this, SLOT(onOrientationChanged()));
}

void TaskCenterLine::onOrientationChanged()
{
    if (ui->rbVertical->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::VERTICAL;
    else if (ui->rbHorizontal->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::HORIZONTAL;
    else if (ui->rbAligned->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::ALIGNED;
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onShiftHorizChanged()
{
    m_cl->m_hShift = ui->qsbHorizShift->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onShiftVertChanged()
{
    m_cl->m_vShift = ui->qsbVertShift->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onRotationChanged()
{
    m_cl->m_rotate = ui->qsbRotate->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onExtendChanged()
{
    m_cl->m_extendBy = ui->qsbExtend->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onColorChanged()
{
    App::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());
    m_cl->m_format.m_color.setValue<QColor>(ui->cpLineColor->color());
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onWeightChanged()
{
    m_cl->m_format.m_weight = ui->dsbWeight->value().getValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onStyleChanged()
{
    m_cl->m_format.m_style = ui->cboxStyle->currentIndex() + 1;
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onFlipChanged()
{
    m_cl->m_flip2Line = ui->cbFlip->isChecked();
    m_partFeat->recomputeFeature();
}


//******************************************************************************
void TaskCenterLine::createCenterLine(void)
{
//    Base::Console().Message("TCL::createCenterLine() - m_type: %d\n", m_type);
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create CenterLine"));
//    bool vertical = false;
    double hShift = ui->qsbHorizShift->rawValue();
    double vShift = ui->qsbVertShift->rawValue();
    double rotate = ui->qsbRotate->rawValue();
    double extendBy = ui->qsbExtend->rawValue();
    std::pair<Base::Vector3d, Base::Vector3d> ends;
    if (ui->rbVertical->isChecked()) {
        m_mode = CenterLine::CLMODE::VERTICAL;
//        vertical = true;
    } else if (ui->rbHorizontal->isChecked()) {
        m_mode = CenterLine::CLMODE::HORIZONTAL;
    } else if (ui->rbAligned->isChecked()) {
        m_mode = CenterLine::CLMODE::ALIGNED;
    }

    bool flip = ui->cbFlip->isChecked();
    TechDraw::CenterLine* cl = CenterLine::CenterLineBuilder(m_partFeat,
                                                             m_subNames,
                                                             m_mode,
                                                             flip);
    if (cl != nullptr) {
        cl->setShifts(hShift, vShift);
        cl->setExtend(extendBy);
        cl->setRotate(rotate);
        cl->m_flip2Line = ui->cbFlip->isChecked();
        App::Color ac;
        ac.setValue<QColor>(ui->cpLineColor->color());
        cl->m_format.m_color = ac;
        cl->m_format.m_weight = ui->dsbWeight->value().getValue();
        cl->m_format.m_style = ui->cboxStyle->currentIndex() + 1;  //Qt Styles start at 0:NoLine
        cl->m_format.m_visible = true;
        m_partFeat->addCenterLine(cl);
    } else {
        Base::Console().Log("TCL::createCenterLine - CenterLine creation failed!\n");
    }

    m_partFeat->recomputeFeature();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();
}

void TaskCenterLine::updateCenterLine(void)
{
//    Base::Console().Message("TCL::updateCenterLine()\n");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Edit CenterLine"));
    m_cl->m_format.m_color.setValue<QColor>(ui->cpLineColor->color() );
    m_cl->m_format.m_weight = ui->dsbWeight->value().getValue();
    m_cl->m_format.m_style = ui->cboxStyle->currentIndex() + 1;
    m_cl->m_format.m_visible = true;

    if (ui->rbVertical->isChecked()) {
        m_mode = CenterLine::CLMODE::VERTICAL;
    } else if (ui->rbHorizontal->isChecked()) {
        m_mode = CenterLine::CLMODE::HORIZONTAL;
    } else if (ui->rbAligned->isChecked()) {
        m_mode = CenterLine::CLMODE::ALIGNED;
    }
    m_cl->m_mode = m_mode;
    m_cl->m_rotate = ui->qsbRotate->rawValue();
    m_cl->m_vShift = ui->qsbVertShift->rawValue();
    m_cl->m_hShift = ui->qsbHorizShift->rawValue();
    m_cl->m_extendBy = ui->qsbExtend->rawValue();
    m_cl->m_type = m_type;
    m_cl->m_flip2Line = ui->cbFlip->isChecked();
    m_partFeat->refreshCLGeoms();
    m_partFeat->requestPaint();

    Gui::Command::updateActive();
    Gui::Command::commitCommand();
}

void TaskCenterLine::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskCenterLine::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

double TaskCenterLine::getCenterWidth()
{
    int lgNumber = Preferences::lineGroup();
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgNumber);

    double width = lg->getWeight("Graphic");
    delete lg;
    Gui::ViewProvider* vp = QGIView::getViewProvider(m_partFeat);
    auto partVP = dynamic_cast<ViewProviderViewPart*>(vp);
    if ( partVP != nullptr ) {
        width = partVP->IsoWidth.getValue();
    }
    return width;
}

Qt::PenStyle TaskCenterLine::getCenterStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    Qt::PenStyle centerStyle = static_cast<Qt::PenStyle> (hGrp->GetInt("CosmoCLStyle", 2));
    return centerStyle;
}

QColor TaskCenterLine::getCenterColor()
{
    return PreferencesGui::centerQColor();
}

double TaskCenterLine::getExtendBy(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    double ext = hGrp->GetFloat("CosmoCLExtend", 3.0);
    return ext;
}

//******************************************************************************

bool TaskCenterLine::accept()
{
//    Base::Console().Message("TCL::accept()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (!getCreateMode())  {
        updateCenterLine();
    } else {
        createCenterLine();
    }
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskCenterLine::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (getCreateMode() &&
        (m_partFeat != nullptr) )  {
//        Base::Console().Message("TCL::reject - create Mode!!\n");
        //nothing to remove.
    }

    if (!getCreateMode() &&
        (m_partFeat != nullptr) )  {
//        Base::Console().Message("TCL::reject - edit Mode!!\n");
          //nothing to un-update
    }

    //make sure any dangling objects are cleaned up
    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgCenterLine::TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                                     TechDraw::DrawPage* page,
                                     std::vector<std::string> subNames,
                                     bool editMode)
    : TaskDialog()
{
    widget  = new TaskCenterLine(partFeat,page,subNames, editMode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-facecenterline"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCenterLine::TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                                     TechDraw::DrawPage* page,
                                     std::string edgeName,
                                     bool editMode)
    : TaskDialog()
{
    widget  = new TaskCenterLine(partFeat,page, edgeName, editMode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-facecenterline"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCenterLine::~TaskDlgCenterLine()
{
}

void TaskDlgCenterLine::update()
{
//    widget->updateTask();
}

void TaskDlgCenterLine::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgCenterLine::open()
{
}

void TaskDlgCenterLine::clicked(int)
{
}

bool TaskDlgCenterLine::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgCenterLine::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskCenterLine.cpp>
