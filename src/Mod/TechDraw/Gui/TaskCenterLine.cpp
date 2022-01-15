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
    m_type(0),           // 0 - Face, 1 - Lines, 2 - Points
    m_mode(0),           // 0 - vertical, 1 - horizontal, 2 - aligned
    m_editMode(editMode)
{
    ui->setupUi(this);

    m_geomIndex = DrawUtil::getIndexFromName(m_edgeName);
    const TechDraw::BaseGeomPtrVector &geoms = partFeat->getEdgeGeometry();
    BaseGeomPtr bg = geoms.at(m_geomIndex);
    std::string tag = bg->getCosmeticTag();
    m_cl = partFeat->getCenterLine(tag);
    //existence of m_cl is checked in CommandAnnotate
    m_type = m_cl->m_type;
   m_mode = m_cl->m_mode;

    setUiEdit();
    // connect the dialog objects
    setUiConnect();
    // save the existing centerline to restore in case the user rejects the changes
    orig_cl = *m_cl;
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
    m_geomIndex(0),
    m_cl(nullptr),
    m_type(0),           // 0 - Face, 1 - Lines, 2 - Points
    m_mode(0),           // 0 - vertical, 1 - horizontal, 2 - aligned
    m_editMode(editMode)
{
    //existence of page and feature are checked by isActive method of calling command

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

    // setup the Ui using (user-defined) default values
    setUiPrimary();
    // connect the dialog objects
    setUiConnect();
    // now create the centerline
    createCenterLine();
}

TaskCenterLine::~TaskCenterLine()
{
}

void TaskCenterLine::updateTask()
{
}

void TaskCenterLine::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskCenterLine::setUiConnect()
{
    // first enabling/disabling
    if (m_type == 0) // if face, then aligned is not possible
        ui->rbAligned->setEnabled(false);
    else
        ui->rbAligned->setEnabled(true);

    // now connection
    connect(ui->cpLineColor, SIGNAL(changed()), this, SLOT(onColorChanged()));
    connect(ui->dsbWeight, SIGNAL(valueChanged(double)), this, SLOT(onWeightChanged()));
    connect(ui->cboxStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(onStyleChanged()));
    connect(ui->qsbVertShift, SIGNAL(valueChanged(double)), this, SLOT(onShiftVertChanged()));
    connect(ui->qsbHorizShift, SIGNAL(valueChanged(double)), this, SLOT(onShiftHorizChanged()));
    connect(ui->qsbExtend, SIGNAL(valueChanged(double)), this, SLOT(onExtendChanged()));
    connect(ui->qsbRotate, SIGNAL(valueChanged(double)), this, SLOT(onRotationChanged()));
    connect(ui->bgOrientation, SIGNAL(buttonClicked(int)), this, SLOT(onOrientationChanged()));
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
    ui->dsbWeight->setValue(m_cl->m_format.m_weight);
    ui->cboxStyle->setCurrentIndex(m_cl->m_format.m_style - 1);

    ui->rbVertical->setChecked(false);
    ui->rbHorizontal->setChecked(false);
    ui->rbAligned->setChecked(false);
    if (m_cl->m_mode == 0)
        ui->rbVertical->setChecked(true);
    else if (m_cl->m_mode == 1)
        ui->rbHorizontal->setChecked(true);
    else if (m_cl->m_mode ==2)
        ui->rbAligned->setChecked(true);

    Base::Quantity qVal;
    qVal.setUnit(Base::Unit::Length);
    qVal.setValue(m_cl->m_vShift);
    ui->qsbVertShift->setValue(qVal);
    qVal.setValue(m_cl->m_hShift);
    ui->qsbHorizShift->setValue(qVal);
    qVal.setValue(m_cl->m_extendBy);
    ui->qsbExtend->setValue(qVal);

    Base::Quantity qAngle;
    qAngle.setUnit(Base::Unit::Angle);
    ui->qsbRotate->setValue(qAngle);
    int precision = Base::UnitsApi::getDecimals();
    ui->qsbRotate->setDecimals(precision);
    ui->qsbRotate->setValue(m_cl->m_rotate);  
}

void TaskCenterLine::onOrientationChanged()
{
    if (ui->rbVertical->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::VERTICAL;
    else if (ui->rbHorizontal->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::HORIZONTAL;
    else if (ui->rbAligned->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::ALIGNED;
    // for centerlines between 2 lines we cannot just recompute
    // because the new orientation might lead to an invalid centerline
    if (m_type == 1)
        updateOrientation();
    else
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

//******************************************************************************
void TaskCenterLine::createCenterLine(void)
{
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create CenterLine"));

    CenterLine* cl = CenterLine::CenterLineBuilder(m_partFeat, m_subNames, m_mode, false);

    // the centerline creation can fail if m_type is edge and both selected edges are horizontal
    // because we attempt by default to create a vertical centerline

    if (cl == nullptr) { // try a horizontal line
        cl = CenterLine::CenterLineBuilder(m_partFeat, m_subNames, CenterLine::CLMODE::HORIZONTAL, false);
        if (cl != nullptr) {
            m_mode = CenterLine::CLMODE::HORIZONTAL;
            ui->rbHorizontal->blockSignals(true);
            ui->rbHorizontal->setChecked(true);
            ui->rbHorizontal->blockSignals(false);
        }
    }

    if (cl != nullptr) {
        double hShift = ui->qsbHorizShift->rawValue();
        double vShift = ui->qsbVertShift->rawValue();
        double rotate = ui->qsbRotate->rawValue();
        double extendBy = ui->qsbExtend->rawValue();
        cl->setShifts(hShift, vShift);
        cl->setExtend(extendBy);
        cl->setRotate(rotate);
        cl->m_flip2Line = false;
        App::Color ac;
        ac.setValue<QColor>(ui->cpLineColor->color());
        cl->m_format.m_color = ac;
        cl->m_format.m_weight = ui->dsbWeight->value().getValue();
        cl->m_format.m_style = ui->cboxStyle->currentIndex() + 1;  //Qt Styles start at 0:NoLine
        cl->m_format.m_visible = true;
        m_partFeat->addCenterLine(cl);
    } else {
        Base::Console().Log("TCL::createCenterLine - CenterLine creation failed!\n");
        Gui::Command::abortCommand();
        return;
    }

    m_partFeat->recomputeFeature();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();

    // entering the edit mode
    m_editMode = true;
    m_cl = cl;
}

void TaskCenterLine::updateOrientation(void)
{
    // When the orientation was changed, it can be that the centerline becomes invalid
    // this can lead to a crash, see e.g.
    // https://forum.freecadweb.org/viewtopic.php?f=35&t=44255&start=20#p503220
    // The centerline creation can fail if m_type is edge and both selected edges are vertical or horizontal.
    // To test the validity before an existing centerline is changed, we create a new one with the desired parameters.
    int orientation = m_cl->m_mode;
    if (!m_edgeName.empty()) { // we have an existing centerline, not a freshly created one
        // since m_subNames is then empty, fill it with two times the centerline
        // because the result of CenterLineBuilder will then in case of success again be the centerline
        m_subNames.resize(2);
        m_subNames[0] = m_edgeName;
        m_subNames[1] = m_edgeName;
    }

    CenterLine* cl = CenterLine::CenterLineBuilder(m_partFeat, m_subNames, orientation, m_cl->m_flip2Line);

    if (cl == nullptr) { // try another orientation
        if (orientation == CenterLine::CLMODE::VERTICAL)
            orientation = CenterLine::CLMODE::HORIZONTAL;
        else if (orientation == CenterLine::CLMODE::HORIZONTAL)
            orientation = CenterLine::CLMODE::VERTICAL;
        cl = CenterLine::CenterLineBuilder(m_partFeat, m_subNames, orientation, m_cl->m_flip2Line);
        if (cl != nullptr) {
            if (orientation == CenterLine::CLMODE::VERTICAL) {
                m_cl->m_mode = CenterLine::CLMODE::VERTICAL;
                ui->rbVertical->blockSignals(true);
                ui->rbVertical->setChecked(true);
                // we know now that only vertical is possible
                ui->rbHorizontal->setEnabled(false);
                ui->rbVertical->blockSignals(false);
            }
            else if (orientation == CenterLine::CLMODE::HORIZONTAL) {
                m_cl->m_mode = CenterLine::CLMODE::HORIZONTAL;
                ui->rbHorizontal->blockSignals(true);
                ui->rbHorizontal->setChecked(true);
                ui->rbVertical->setEnabled(false);
                ui->rbHorizontal->blockSignals(false);
            }
        }
    }

    if (cl != nullptr) { // we succeeded
        // reset the flip for existing centerline that might use the flip feature (when created with FC 0.19)
        m_cl->m_flip2Line = false;
        m_partFeat->recomputeFeature();
    }
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
    Qt::PenStyle centerStyle = static_cast<Qt::PenStyle> (hGrp->GetInt("CenterLine", 2));
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
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc)
        return false;

    Gui::Command::updateActive();
    Gui::Command::commitCommand();
    doc->resetEdit();

    return true;
}

bool TaskCenterLine::reject()
{
    Gui::Command::abortCommand();

    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc)
        return false;

    if (getCreateMode() && m_partFeat)  {
        // undo the centerline creation
        doc->undo(1);
    }
    else if (!getCreateMode() && m_partFeat) {
        // restore the initial centerline
        m_cl->m_format.m_color = (&orig_cl)->m_format.m_color;
        m_cl->m_format.m_weight = (&orig_cl)->m_format.m_weight;
        m_cl->m_format.m_style = (&orig_cl)->m_format.m_style;
        m_cl->m_format.m_visible = (&orig_cl)->m_format.m_visible;
        m_cl->m_mode = (&orig_cl)->m_mode;
        m_cl->m_rotate = (&orig_cl)->m_rotate;
        m_cl->m_vShift = (&orig_cl)->m_vShift;
        m_cl->m_hShift = (&orig_cl)->m_hShift;
        m_cl->m_extendBy = (&orig_cl)->m_extendBy;
        m_cl->m_type = (&orig_cl)->m_type;
    }

    if (m_partFeat)
        m_partFeat->recomputeFeature();
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().recompute()");
    doc->resetEdit();

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgCenterLine::TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                                     TechDraw::DrawPage* page,
                                     std::vector<std::string> subNames,
                                     bool editMode)
    : TaskDialog()
{
    widget  = new TaskCenterLine(partFeat,page,subNames, editMode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_FaceCenterLine"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
    setAutoCloseOnTransactionChange(true);
}

TaskDlgCenterLine::TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                                     TechDraw::DrawPage* page,
                                     std::string edgeName,
                                     bool editMode)
    : TaskDialog()
{
    widget  = new TaskCenterLine(partFeat,page, edgeName, editMode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_FaceCenterLine"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
    setAutoCloseOnTransactionChange(true);
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
