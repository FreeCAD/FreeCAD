/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "TaskCenterLine.h"
#include "ui_TaskCenterLine.h"
#include "PreferencesGui.h"
#include "QGIView.h"
#include "ViewProviderViewPart.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

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
    m_type(CenterLine::FACE),
    m_mode(CenterLine::VERTICAL),
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
    m_type(CenterLine::FACE),
    m_mode(CenterLine::VERTICAL),
    m_editMode(editMode)
{
    //existence of page and feature are checked by isActive method of calling command

    ui->setupUi(this);
    std::string check = subNames.front();
    std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(check);
    if (geomType == "Face") {
        m_type = CenterLine::FACE;
    } else if (geomType == "Edge") {
        m_type = CenterLine::EDGE;
    } else if (geomType == "Vertex") {
        m_type = CenterLine::VERTEX;
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

void TaskCenterLine::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskCenterLine::setUiConnect()
{
    // first enabling/disabling
    if (m_type == CenterLine::FACE) // if face, then aligned is not possible
        ui->rbAligned->setEnabled(false);
    else
        ui->rbAligned->setEnabled(true);

    // now connection
    connect(ui->cpLineColor, &ColorButton::changed, this, &TaskCenterLine::onColorChanged);
    connect(ui->dsbWeight, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskCenterLine::onWeightChanged);
    connect(ui->cboxStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskCenterLine::onStyleChanged);
    connect(ui->qsbVertShift, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskCenterLine::onShiftVertChanged);
    connect(ui->qsbHorizShift, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskCenterLine::onShiftHorizChanged);
    connect(ui->qsbExtend, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskCenterLine::onExtendChanged);
    connect(ui->qsbRotate, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskCenterLine::onRotationChanged);
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    connect(ui->bgOrientation, qOverload<int>(&QButtonGroup::buttonClicked), this, &TaskCenterLine::onOrientationChanged);
#else
    connect(ui->bgOrientation, &QButtonGroup::idClicked, this, &TaskCenterLine::onOrientationChanged);
#endif
}

void TaskCenterLine::setUiPrimary()
{
    setWindowTitle(QObject::tr("Create Center Line"));

    if (m_partFeat) {
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

    if (m_type == CenterLine::EDGE) {
       int orientation = checkPathologicalEdges(m_mode);
       setUiOrientation(orientation);
    }
    if (m_type == CenterLine::VERTEX) {
       int orientation = checkPathologicalVertices(m_mode);
       setUiOrientation(orientation);
    }
}

void TaskCenterLine::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Center Line"));
    if (m_partFeat) {
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
    if (m_cl->m_mode == CenterLine::VERTICAL)
        ui->rbVertical->setChecked(true);
    else if (m_cl->m_mode == CenterLine::HORIZONTAL)
        ui->rbHorizontal->setChecked(true);
    else if (m_cl->m_mode == CenterLine::ALIGNED)
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
    if (!m_cl) {
        return;
    }
    if (ui->rbVertical->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::VERTICAL;
    else if (ui->rbHorizontal->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::HORIZONTAL;
    else if (ui->rbAligned->isChecked())
        m_cl->m_mode = CenterLine::CLMODE::ALIGNED;
    // for centerlines between 2 lines we cannot just recompute
    // because the new orientation might lead to an invalid centerline
    if (m_type == CenterLine::EDGE)
        updateOrientation();
    else
        m_partFeat->recomputeFeature();
}

void TaskCenterLine::onShiftHorizChanged()
{
    if (!m_cl) {
        return;
    }

    m_cl->m_hShift = ui->qsbHorizShift->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onShiftVertChanged()
{
    if (!m_cl) {
        return;
    }

    m_cl->m_vShift = ui->qsbVertShift->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onRotationChanged()
{
    if (!m_cl) {
        return;
    }

    m_cl->m_rotate = ui->qsbRotate->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onExtendChanged()
{
    if (!m_cl) {
        return;
    }

    m_cl->m_extendBy = ui->qsbExtend->rawValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onColorChanged()
{
    if (!m_cl) {
        return;
    }

    App::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());
    m_cl->m_format.m_color.setValue<QColor>(ui->cpLineColor->color());
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onWeightChanged()
{
    if (!m_cl) {
        return;
    }

    m_cl->m_format.m_weight = ui->dsbWeight->value().getValue();
    m_partFeat->recomputeFeature();
}

void TaskCenterLine::onStyleChanged()
{
    if (!m_cl) {
        return;
    }

    m_cl->m_format.m_style = ui->cboxStyle->currentIndex() + 1;
    m_partFeat->recomputeFeature();
}

// check that we are not trying to create an impossible centerline (ex a vertical centerline
// between 2 horizontal edges)
int TaskCenterLine::checkPathologicalEdges(int inMode)
{
    if (m_type != CenterLine::EDGE) {
        // not an edge based centerline, this doesn't apply
        return inMode;
    }

    TechDraw::BaseGeomPtr edge1 = m_partFeat->getEdge(m_subNames.front());
    std::vector<Base::Vector3d> ends1 = edge1->findEndPoints();
    bool edge1Vertical = DU::fpCompare(ends1.front().x, ends1.back().x, EWTOLERANCE);
    bool edge1Horizontal = DU::fpCompare(ends1.front().y, ends1.back().y, EWTOLERANCE);

    TechDraw::BaseGeomPtr edge2 = m_partFeat->getEdge(m_subNames.back());
    std::vector<Base::Vector3d> ends2 = edge2->findEndPoints();
    bool edge2Vertical = DU::fpCompare(ends2.front().x, ends2.back().x, EWTOLERANCE);
    bool edge2Horizontal = DU::fpCompare(ends2.front().y, ends2.back().y, EWTOLERANCE);

    if (edge1Vertical && edge2Vertical) {
        return CenterLine::CLMODE::VERTICAL;
    }
    if (edge1Horizontal && edge2Horizontal) {
        return CenterLine::CLMODE::HORIZONTAL;
    }

    // not pathological case, just return the input mode
    return inMode;
}

// check that we are not trying to create an impossible centerline (ex a vertical centerline
// between 2 vertices aligned vertically)
int TaskCenterLine::checkPathologicalVertices(int inMode)
{
    if (m_type != CenterLine::VERTEX) {
        // not a vertex based centerline, this doesn't apply
        return inMode;
    }

    TechDraw::VertexPtr vert1 = m_partFeat->getVertex(m_subNames.front());
    Base::Vector3d point1 = vert1->point();
    TechDraw::VertexPtr vert2 = m_partFeat->getVertex(m_subNames.back());
    Base::Vector3d point2 = vert2->point();

    if (DU::fpCompare(point1.x, point2.x, EWTOLERANCE)) {
        // points are aligned vertically, CL must be horizontal
        return CenterLine::CLMODE::HORIZONTAL;
    }

    if (DU::fpCompare(point1.y, point2.y, EWTOLERANCE)) {
        // points are aligned horizontally, CL must be vertical
        return CenterLine::CLMODE::VERTICAL;
    }

    // not pathological case, just return the input mode
    return inMode;
}

//******************************************************************************
void TaskCenterLine::createCenterLine()
{
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create CenterLine"));

    // check for illogical parameters
    if (m_type == CenterLine::EDGE) {
        // between lines
        m_mode = checkPathologicalEdges(m_mode);
    } else if (m_type == CenterLine::VERTEX) {
        // between points
        m_mode = checkPathologicalVertices(m_mode);
    }

    CenterLine* cl = CenterLine::CenterLineBuilder(m_partFeat, m_subNames, m_mode, false);

    if (!cl) {
        Gui::Command::abortCommand();
        return;
    }

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

    m_partFeat->recomputeFeature();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();

    // entering the edit mode
    m_editMode = true;
    m_cl = cl;
}

void TaskCenterLine::updateOrientation()
{
//    Base::Console().Message("TCL::updateOrientation()\n");
    if (!m_cl) {
        return;
    }
    // When the orientation was changed, it can be that the centerline becomes invalid
    // this can lead to a crash, see e.g.
    // https://forum.freecad.org/viewtopic.php?f=35&t=44255&start=20#p503220
    // The centerline creation can fail if m_type is edge and both selected edges are vertical or horizontal.
    int orientation = m_cl->m_mode;
    if (m_type == CenterLine::EDGE) {
        // between lines
        if (!m_edgeName.empty() && !m_cl->m_edges.empty()) {
             // we have an existing centerline, not a freshly created one, and it is a centerline between edges
            m_subNames = m_cl->m_edges;
            orientation = checkPathologicalEdges(orientation);
        }
    } else if (m_type == CenterLine::VERTEX) {
        // between points
        if (!m_edgeName.empty() && !m_cl->m_verts.empty()) {
             // we have an existing centerline, not a freshly created one, and it is a centerline between points
            m_subNames = m_cl->m_verts;
            orientation = checkPathologicalVertices(orientation);
        }
    }

    setUiOrientation(orientation);

    m_partFeat->recomputeFeature();
}

void TaskCenterLine::setUiOrientation(int orientation)
{
    ui->rbVertical->blockSignals(true);
    ui->rbVertical->blockSignals(true);

    if (orientation == CenterLine::CLMODE::VERTICAL) {
        ui->rbVertical->setChecked(true);
        ui->rbHorizontal->setChecked(false);
    } else if (orientation == CenterLine::CLMODE::HORIZONTAL) {
        ui->rbVertical->setChecked(false);
        ui->rbHorizontal->setChecked(true);
    }

    ui->rbVertical->blockSignals(false);
    ui->rbVertical->blockSignals(false);

}

void TaskCenterLine::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskCenterLine::enableTaskButtons(bool isEnabled)
{
    m_btnOK->setEnabled(isEnabled);
    m_btnCancel->setEnabled(isEnabled);
}

double TaskCenterLine::getCenterWidth()
{
    Gui::ViewProvider* vp = QGIView::getViewProvider(m_partFeat);
    auto partVP = dynamic_cast<ViewProviderViewPart*>(vp);
    if (!partVP) {
        return TechDraw::LineGroup::getDefaultWidth("Graphic");
    }
    return partVP->IsoWidth.getValue();
}

Qt::PenStyle TaskCenterLine::getCenterStyle()
{
    Qt::PenStyle centerStyle = static_cast<Qt::PenStyle> (Preferences::getPreferenceGroup("Decorations")->GetInt("CenterLine", 2));
    return centerStyle;
}

QColor TaskCenterLine::getCenterColor()
{
    return PreferencesGui::centerQColor();
}

double TaskCenterLine::getExtendBy()
{
    return Preferences::getPreferenceGroup("Decorations")->GetFloat("CosmoCLExtend", 3.0);
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
    widget  = new TaskCenterLine(partFeat, page, subNames, editMode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_FaceCenterLine"),
                                             widget->windowTitle(), true, nullptr);
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
    widget  = new TaskCenterLine(partFeat, page, edgeName, editMode);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_FaceCenterLine"),
                                             widget->windowTitle(), true, nullptr);
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
