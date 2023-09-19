/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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
#include <cassert>
#include <limits>
#include <QApplication>
#include <QGridLayout>
#endif

#include <App/Document.h>
#include "Document.h" // must be before TaskCSysDragger.h
#include "TaskCSysDragger.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "QuantitySpinBox.h"
#include "SoFCCSysDragger.h"
#include "ViewProviderDragger.h"
#include "TaskView/TaskView.h"


using namespace Gui;


static double degreesToRadians(const double &degreesIn)
{
  return degreesIn * (M_PI / 180.0);
}


TaskCSysDragger::TaskCSysDragger(Gui::ViewProviderDocumentObject* vpObjectIn, Gui::SoFCCSysDragger* draggerIn) :
  dragger(draggerIn)
{
  assert(vpObjectIn);
  assert(draggerIn);
  vpObject = vpObjectIn->getObject();
  dragger->ref();

  setupGui();
}

TaskCSysDragger::~TaskCSysDragger()
{
  dragger->unref();
  Gui::Application::Instance->commandManager().getCommandByName("Std_OrthographicCamera")->setEnabled(true);
  Gui::Application::Instance->commandManager().getCommandByName("Std_PerspectiveCamera")->setEnabled(true);
}

void TaskCSysDragger::dragStartCallback(void *, SoDragger *)
{
    // This is called when a manipulator is about to manipulating
  if(firstDrag)
    {
       Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Transform"));
       firstDrag=false;
    }
}

void TaskCSysDragger::setupGui()
{
    auto incrementsBox = new Gui::TaskView::TaskBox(
      Gui::BitmapFactory().pixmap("button_valid"),
      tr("Increments"), true, nullptr);

    auto gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(1, 1);

  auto tLabel = new QLabel(tr("Translation Increment:"), incrementsBox);
  gridLayout->addWidget(tLabel, 0, 0, Qt::AlignRight);

  QFontMetrics metrics(QApplication::font());
  int spinBoxWidth = metrics.averageCharWidth() * 20;
  tSpinBox = new QuantitySpinBox(incrementsBox);
  tSpinBox->setMinimum(0.0);
  tSpinBox->setMaximum(std::numeric_limits<double>::max());
  tSpinBox->setUnit(Base::Unit::Length);
  tSpinBox->setMinimumWidth(spinBoxWidth);
  gridLayout->addWidget(tSpinBox, 0, 1, Qt::AlignLeft);

  auto rLabel = new QLabel(tr("Rotation Increment:"), incrementsBox);
  gridLayout->addWidget(rLabel, 1, 0, Qt::AlignRight);

  rSpinBox = new QuantitySpinBox(incrementsBox);
  rSpinBox->setMinimum(0.0);
  rSpinBox->setMaximum(180.0);
  rSpinBox->setUnit(Base::Unit::Angle);
  rSpinBox->setMinimumWidth(spinBoxWidth);
  gridLayout->addWidget(rSpinBox, 1, 1, Qt::AlignLeft);

  incrementsBox->groupLayout()->addLayout(gridLayout);
  Content.push_back(incrementsBox);

  connect(tSpinBox, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskCSysDragger::onTIncrementSlot);
  connect(rSpinBox, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskCSysDragger::onRIncrementSlot);
}

void TaskCSysDragger::onTIncrementSlot(double freshValue)
{
  dragger->translationIncrement.setValue(freshValue);
}

void TaskCSysDragger::onRIncrementSlot(double freshValue)
{
  dragger->rotationIncrement.setValue(degreesToRadians(freshValue));
}

void TaskCSysDragger::open()
{
  dragger->addStartCallback(dragStartCallback, this);
  //we can't have user switching camera types while dragger is shown.
  Gui::Application::Instance->commandManager().getCommandByName("Std_OrthographicCamera")->setEnabled(false);
  Gui::Application::Instance->commandManager().getCommandByName("Std_PerspectiveCamera")->setEnabled(false);
//   dragger->translationIncrement.setValue(lastTranslationIncrement);
//   dragger->rotationIncrement.setValue(lastRotationIncrement);
  ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/History/Dragger");
  double lastTranslationIncrement = hGrp->GetFloat("LastTranslationIncrement", 1.0);
  double lastRotationIncrement = hGrp->GetFloat("LastRotationIncrement", 15.0);
  tSpinBox->setValue(lastTranslationIncrement);
  rSpinBox->setValue(lastRotationIncrement);

  Gui::TaskView::TaskDialog::open();
}

bool TaskCSysDragger::accept()
{
  ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/History/Dragger");
  hGrp->SetFloat("LastTranslationIncrement", tSpinBox->rawValue());
  hGrp->SetFloat("LastRotationIncrement", rSpinBox->rawValue());

  App::DocumentObject* dObject = vpObject.getObject();
  if (dObject) {
    Gui::Document* document = Gui::Application::Instance->getDocument(dObject->getDocument());
    assert(document);
    firstDrag = true;
    document->commitCommand();
    document->resetEdit();
    document->getDocument()->recompute();
  }
  return Gui::TaskView::TaskDialog::accept();
}

bool TaskCSysDragger::reject()
{
  App::DocumentObject* dObject = vpObject.getObject();
  if (dObject) {
    Gui::Document* document = Gui::Application::Instance->getDocument(dObject->getDocument());
    assert(document);
    firstDrag = true;
    document->abortCommand();
    document->resetEdit();
    document->getDocument()->recompute();
  }
  return Gui::TaskView::TaskDialog::reject();
}

#include "moc_TaskCSysDragger.cpp"
