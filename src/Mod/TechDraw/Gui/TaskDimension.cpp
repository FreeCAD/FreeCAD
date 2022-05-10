/***************************************************************************
 *   Copyright (c) 2021 Uwe Stöhr <uwestoehr@lyx.org>                      *
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
#include <QMessageBox>
#endif // #ifndef _PreComp_


#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIViewDimension.h"
#include "ViewProviderDimension.h"
#include "ui_TaskDimension.h"
#include "TaskDimension.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskDimension::TaskDimension(QGIViewDimension *parent, ViewProviderDimension *dimensionVP) :
    ui(new Ui_TaskDimension)
{
    m_parent = parent;
    m_dimensionVP = dimensionVP;

    ui->setupUi(this);

    // Tolerancing
    ui->cbTheoreticallyExact->setChecked(parent->dvDimension->TheoreticalExact.getValue());
    connect(ui->cbTheoreticallyExact, SIGNAL(stateChanged(int)), this, SLOT(onTheoreticallyExactChanged()));
    // if TheoreticalExact disable tolerances
    if (parent->dvDimension->TheoreticalExact.getValue()) {
        ui->cbEqualTolerance->setDisabled(true);
        ui->qsbOvertolerance->setDisabled(true);
        ui->qsbUndertolerance->setDisabled(true);
        ui->leFormatSpecifierOverTolerance->setDisabled(true);
        ui->leFormatSpecifierUnderTolerance->setDisabled(true);
    }
    ui->cbEqualTolerance->setChecked(parent->dvDimension->EqualTolerance.getValue());
    connect(ui->cbEqualTolerance, SIGNAL(stateChanged(int)), this, SLOT(onEqualToleranceChanged()));
    // if EqualTolerance overtolernace must not be negative
    if (parent->dvDimension->EqualTolerance.getValue())
        ui->qsbOvertolerance->setMinimum(0.0);
    if ((parent->dvDimension->Type.isValue("Angle")) ||
        (parent->dvDimension->Type.isValue("Angle3Pt"))) {
        ui->qsbOvertolerance->setUnit(Base::Unit::Angle);
        ui->qsbUndertolerance->setUnit(Base::Unit::Angle);
    }
    else {
        ui->qsbOvertolerance->setUnit(Base::Unit::Length);
        ui->qsbUndertolerance->setUnit(Base::Unit::Length);
    }
    ui->qsbOvertolerance->setValue(parent->dvDimension->OverTolerance.getValue());
    ui->qsbUndertolerance->setValue(parent->dvDimension->UnderTolerance.getValue());
    connect(ui->qsbOvertolerance, SIGNAL(valueChanged(double)), this, SLOT(onOvertoleranceChanged()));
    connect(ui->qsbUndertolerance, SIGNAL(valueChanged(double)), this, SLOT(onUndertoleranceChanged()));
    // undertolerance is disabled when EqualTolerance is true
    if (ui->cbEqualTolerance->isChecked()) {
        ui->qsbUndertolerance->setDisabled(true);
        ui->leFormatSpecifierUnderTolerance->setDisabled(true);
    }

    // Formatting
    std::string StringValue = parent->dvDimension->FormatSpec.getValue();
    QString qs = QString::fromUtf8(StringValue.data(), StringValue.size());
    ui->leFormatSpecifier->setText(qs);
    connect(ui->leFormatSpecifier, SIGNAL(textChanged(QString)), this, SLOT(onFormatSpecifierChanged()));
    ui->cbArbitrary->setChecked(parent->dvDimension->Arbitrary.getValue());
    connect(ui->cbArbitrary, SIGNAL(stateChanged(int)), this, SLOT(onArbitraryChanged()));
    StringValue = parent->dvDimension->FormatSpecOverTolerance.getValue();
    qs = QString::fromUtf8(StringValue.data(), StringValue.size());
    ui->leFormatSpecifierOverTolerance->setText(qs);
    StringValue = parent->dvDimension->FormatSpecUnderTolerance.getValue();
    qs = QString::fromUtf8(StringValue.data(), StringValue.size());
    ui->leFormatSpecifierUnderTolerance->setText(qs);
    connect(ui->leFormatSpecifierOverTolerance, SIGNAL(textChanged(QString)), this, SLOT(onFormatSpecifierOverToleranceChanged()));
    connect(ui->leFormatSpecifierUnderTolerance, SIGNAL(textChanged(QString)), this, SLOT(onFormatSpecifierUnderToleranceChanged()));
    ui->cbArbitraryTolerances->setChecked(parent->dvDimension->ArbitraryTolerances.getValue());
    connect(ui->cbArbitraryTolerances, SIGNAL(stateChanged(int)), this, SLOT(onArbitraryTolerancesChanged()));

    // Display Style
    if (dimensionVP != nullptr) {
        ui->cbArrowheads->setChecked(dimensionVP->FlipArrowheads.getValue());
        connect(ui->cbArrowheads, SIGNAL(stateChanged(int)), this, SLOT(onFlipArrowheadsChanged()));
        ui->dimensionColor->setColor(dimensionVP->Color.getValue().asValue<QColor>());
        connect(ui->dimensionColor, SIGNAL(changed()), this, SLOT(onColorChanged()));
        ui->qsbFontSize->setValue(dimensionVP->Fontsize.getValue());
        ui->qsbFontSize->setUnit(Base::Unit::Length);
        ui->qsbFontSize->setMinimum(0);
        connect(ui->qsbFontSize, SIGNAL(valueChanged(double)), this, SLOT(onFontsizeChanged()));
        ui->comboDrawingStyle->setCurrentIndex(dimensionVP->StandardAndStyle.getValue());
        connect(ui->comboDrawingStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(onDrawingStyleChanged()));
    }

    // Lines
    ui->rbOverride->setChecked(parent->dvDimension->AngleOverride.getValue());
    connect(ui->rbOverride, SIGNAL(toggled(bool)), this, SLOT(onOverrideToggled()));
    ui->dsbDimAngle->setValue(parent->dvDimension->LineAngle.getValue());
    connect(ui->dsbDimAngle, SIGNAL(valueChanged(double)), this, SLOT(onDimAngleChanged()));
    ui->dsbExtAngle->setValue(parent->dvDimension->ExtensionAngle.getValue());
    connect(ui->dsbExtAngle, SIGNAL(valueChanged(double)), this, SLOT(onExtAngleChanged()));
    connect(ui->pbDimUseDefault, SIGNAL(clicked()), this, SLOT(onDimUseDefaultClicked()));
    connect(ui->pbDimUseSelection, SIGNAL(clicked()), this, SLOT(onDimUseSelectionClicked()));
    connect(ui->pbExtUseDefault, SIGNAL(clicked()), this, SLOT(onExtUseDefaultClicked()));
    connect(ui->pbExtUseSelection, SIGNAL(clicked()), this, SLOT(onExtUseSelectionClicked()));

    Gui::Document* doc = m_dimensionVP->getDocument();
    doc->openCommand("TaskDimension");
}

TaskDimension::~TaskDimension()
{
}

bool TaskDimension::accept()
{
    Gui::Document* doc = m_dimensionVP->getDocument();
    m_dimensionVP->getObject()->purgeTouched();
    doc->commitCommand();
    doc->resetEdit();

    return true;
}

bool TaskDimension::reject()
{
    Gui::Document* doc = m_dimensionVP->getDocument();
    doc->abortCommand();
    recomputeFeature();
    m_parent->updateView(true);
    m_dimensionVP->getObject()->purgeTouched();
    doc->resetEdit();

    return true;
}

void TaskDimension::recomputeFeature()
{
    App::DocumentObject* objVP = m_dimensionVP->getObject();
    assert(objVP);
    objVP->getDocument()->recomputeFeature(objVP);
}

void TaskDimension::onTheoreticallyExactChanged()
{
    m_parent->dvDimension->TheoreticalExact.setValue(ui->cbTheoreticallyExact->isChecked());
    // if TheoreticalExact disable tolerances and set them to zero
    if (ui->cbTheoreticallyExact->isChecked()) {
        ui->qsbOvertolerance->setValue(0.0);
        ui->qsbUndertolerance->setValue(0.0);
        ui->cbEqualTolerance->setDisabled(true);
        ui->qsbOvertolerance->setDisabled(true);
        ui->qsbUndertolerance->setDisabled(true);
        ui->leFormatSpecifierOverTolerance->setDisabled(true);
        ui->leFormatSpecifierUnderTolerance->setDisabled(true);
        ui->cbArbitraryTolerances->setDisabled(true);
        ui->cbArbitraryTolerances->setChecked(false);
    }
    else {
        ui->cbEqualTolerance->setDisabled(false);
        ui->qsbOvertolerance->setDisabled(false);
        ui->leFormatSpecifierOverTolerance->setDisabled(false);
        ui->cbArbitraryTolerances->setDisabled(false);
        if (!ui->cbEqualTolerance->isChecked()) {
            ui->qsbUndertolerance->setDisabled(false);
            ui->leFormatSpecifierUnderTolerance->setDisabled(false);
        }
    }
    recomputeFeature();
}

void TaskDimension::onEqualToleranceChanged()
{
    m_parent->dvDimension->EqualTolerance.setValue(ui->cbEqualTolerance->isChecked());
    // if EqualTolerance set negated overtolerance for untertolerance
    // then also the OverTolerance must be positive
    if (ui->cbEqualTolerance->isChecked()) {
        // if OverTolerance is negative or zero, first set it to zero
        if (ui->qsbOvertolerance->value().getValue() < 0)
            ui->qsbOvertolerance->setValue(0.0);
        ui->qsbOvertolerance->setMinimum(0.0);
        ui->qsbUndertolerance->setValue(-1.0 * ui->qsbOvertolerance->value().getValue());
        ui->qsbUndertolerance->setUnit(ui->qsbOvertolerance->value().getUnit());
        ui->qsbUndertolerance->setDisabled(true);
        ui->leFormatSpecifierUnderTolerance->setDisabled(true);
    }
    else {
        ui->qsbOvertolerance->setMinimum(-DBL_MAX);
        if (!ui->cbTheoreticallyExact->isChecked()) {
            ui->qsbUndertolerance->setDisabled(false);
            ui->leFormatSpecifierUnderTolerance->setDisabled(false);
        }
    }
    recomputeFeature();
}

void TaskDimension::onOvertoleranceChanged()
{
    m_parent->dvDimension->OverTolerance.setValue(ui->qsbOvertolerance->value().getValue());
    // if EqualTolerance set negated overtolerance for untertolerance
    if (ui->cbEqualTolerance->isChecked()) {
        ui->qsbUndertolerance->setValue(-1.0 * ui->qsbOvertolerance->value().getValue());
        ui->qsbUndertolerance->setUnit(ui->qsbOvertolerance->value().getUnit());
    }
    recomputeFeature();
}

void TaskDimension::onUndertoleranceChanged()
{
    m_parent->dvDimension->UnderTolerance.setValue(ui->qsbUndertolerance->value().getValue());
    recomputeFeature();
}

void TaskDimension::onFormatSpecifierChanged()
{
    m_parent->dvDimension->FormatSpec.setValue(ui->leFormatSpecifier->text().toUtf8().constData());
    recomputeFeature();
}

void TaskDimension::onArbitraryChanged()
{
    m_parent->dvDimension->Arbitrary.setValue(ui->cbArbitrary->isChecked());
    recomputeFeature();
}

void TaskDimension::onFormatSpecifierOverToleranceChanged()
{
    m_parent->dvDimension->FormatSpecOverTolerance.setValue(ui->leFormatSpecifierOverTolerance->text().toUtf8().constData());
    if (!ui->cbArbitraryTolerances->isChecked()) {
        ui->leFormatSpecifierUnderTolerance->setText(ui->leFormatSpecifierOverTolerance->text());
        m_parent->dvDimension->FormatSpecUnderTolerance.setValue(ui->leFormatSpecifierUnderTolerance->text().toUtf8().constData());
    }
    recomputeFeature();
}

void TaskDimension::onFormatSpecifierUnderToleranceChanged()
{
    m_parent->dvDimension->FormatSpecUnderTolerance.setValue(ui->leFormatSpecifierUnderTolerance->text().toUtf8().constData());
    if (!ui->cbArbitraryTolerances->isChecked()) {
        ui->leFormatSpecifierOverTolerance->setText(ui->leFormatSpecifierUnderTolerance->text());
        m_parent->dvDimension->FormatSpecOverTolerance.setValue(ui->leFormatSpecifierOverTolerance->text().toUtf8().constData());
    }
    recomputeFeature();
}

void TaskDimension::onArbitraryTolerancesChanged()
{
    m_parent->dvDimension->ArbitraryTolerances.setValue(ui->cbArbitraryTolerances->isChecked());
    recomputeFeature();
}

void TaskDimension::onFlipArrowheadsChanged()
{
    m_dimensionVP->FlipArrowheads.setValue(ui->cbArrowheads->isChecked());
    recomputeFeature();
}

void TaskDimension::onColorChanged()
{
    App::Color ac;
    ac.setValue<QColor>(ui->dimensionColor->color());
    m_dimensionVP->Color.setValue(ac);
    recomputeFeature();
}

void TaskDimension::onFontsizeChanged()
{
    m_dimensionVP->Fontsize.setValue(ui->qsbFontSize->value().getValue());
    recomputeFeature();
}

void TaskDimension::onDrawingStyleChanged()
{
    m_dimensionVP->StandardAndStyle.setValue(ui->comboDrawingStyle->currentIndex());
    recomputeFeature();
}

void TaskDimension::onOverrideToggled()
{
    m_parent->dvDimension->AngleOverride.setValue(ui->rbOverride->isChecked());
    recomputeFeature();

}

void TaskDimension::onDimAngleChanged()
{
    m_parent->dvDimension->LineAngle.setValue(ui->dsbDimAngle->value());
    recomputeFeature();
}

void TaskDimension::onExtAngleChanged()
{
    m_parent->dvDimension->ExtensionAngle.setValue(ui->dsbExtAngle->value());
    recomputeFeature();
}

void TaskDimension::onDimUseDefaultClicked()
{
    pointPair points = m_parent->dvDimension->getLinearPoints();
    //duplicate coordinate conversion logic from QGIViewDimension
    Base::Vector2d first2(points.first.x, -points.first.y);
    Base::Vector2d second2(points.second.x, -points.second.y);
    double lineAngle = (second2 - first2).Angle();
    ui->dsbDimAngle->setValue(lineAngle * 180.0 / M_PI);
}

void TaskDimension::onDimUseSelectionClicked()
{
    std::pair<double, bool> result = getAngleFromSelection();
    if (result.second) {
        ui->dsbDimAngle->setValue(result.first * 180.0 / M_PI);
    }
}

void TaskDimension::onExtUseDefaultClicked()
{
    pointPair points = m_parent->dvDimension->getLinearPoints();
    //duplicate coordinate conversion logic from QGIViewDimension
    Base::Vector2d first2(points.first.x, -points.first.y);
    Base::Vector2d second2(points.second.x, -points.second.y);
    Base::Vector2d lineDirection = second2 - first2;
    Base::Vector2d extensionDirection(-lineDirection.y, lineDirection.x);
    double extensionAngle = extensionDirection.Angle();
    ui->dsbExtAngle->setValue(extensionAngle * 180.0 / M_PI);
}
void TaskDimension::onExtUseSelectionClicked()
{
    std::pair<double, bool> result = getAngleFromSelection();
    if (result.second) {
        ui->dsbExtAngle->setValue(result.first * 180.0 / M_PI);
    }
}

std::pair<double, bool> TaskDimension::getAngleFromSelection()
{
    std::pair<double, bool> result;
    result.first = 0.0;
    result.second = true;
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    TechDraw::DrawViewPart * objFeat = nullptr;
    std::vector<std::string> SubNames;
    if (!selection.empty()) {
        objFeat = static_cast<TechDraw::DrawViewPart*> (selection.front().getObject());
        SubNames = selection.front().getSubNames();
        if (SubNames.size() == 2) {             //expecting Vertices
            std::string geomName0 = DrawUtil::getGeomTypeFromName(SubNames[0]);
            int geomIndex0 = DrawUtil::getIndexFromName(SubNames[0]);
            std::string geomName1 = DrawUtil::getGeomTypeFromName(SubNames[1]);
            int geomIndex1 = DrawUtil::getIndexFromName(SubNames[1]);
            if ((geomName0 == "Vertex") && (geomName1 == "Vertex"))  {
                TechDraw::VertexPtr v0 = objFeat->getProjVertexByIndex(geomIndex0);
                TechDraw::VertexPtr v1 = objFeat->getProjVertexByIndex(geomIndex1);
                Base::Vector2d v02(v0->point().x, -v0->point().y);
                Base::Vector2d v12(v1->point().x, -v1->point().y);
                result.first = (v12 - v02).Angle();
                return result;
            }
        } else if (SubNames.size() == 1) {      //expecting Edge
            std::string geomName0 = DrawUtil::getGeomTypeFromName(SubNames[0]);
            int geomIndex0 = DrawUtil::getIndexFromName(SubNames[0]);
            if (geomName0 == "Edge") {
                TechDraw::BaseGeomPtr edge = objFeat->getGeomByIndex(geomIndex0);
                Base::Vector2d v02(edge->getStartPoint().x, -edge->getStartPoint().y);
                Base::Vector2d v12(edge->getEndPoint().x, -edge->getEndPoint().y);
                result.first = (v12 - v02).Angle();
                return result;
            }
        }
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                               QObject::tr("Select 2 Vertexes or 1 Edge"));
    result.second = false;
    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgDimension::TaskDlgDimension(QGIViewDimension *parent, ViewProviderDimension *dimensionVP) :
    TaskDialog()
{
    widget  = new TaskDimension(parent, dimensionVP);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_Dimension"), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
    setAutoCloseOnTransactionChange(true);
}

TaskDlgDimension::~TaskDlgDimension()
{
}

void TaskDlgDimension::update()
{
}

//==== calls from the TaskView ===============================================================
void TaskDlgDimension::open()
{
}

void TaskDlgDimension::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgDimension::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgDimension::reject()
{
    widget->reject();
    return true;
}

#include "moc_TaskDimension.cpp"
