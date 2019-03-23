/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson         <blobfish[at]gmx.com>      *
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
#include <assert.h>
#include <limits>
#include <QApplication>
#endif

#include <QGridLayout>
#include <QFontMetrics>

#include <Gui/TaskView/TaskView.h>
#include "QuantitySpinBox.h"
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProviderDragger.h>
#include <Gui/SoFCCSysDragger.h>

#include "TaskCSysDragger.h"

using namespace Gui;


static double radiansToDegrees(const double &radiansIn)
{
  return radiansIn * (180.0 / M_PI);
}

static double degreesToRadains(const double &degreesIn)
{
  return degreesIn * (M_PI / 180.0);
}

static double lastTranslationIncrement = 1.0;
static double lastRotationIncrement = degreesToRadains(15.0);

TaskCSysDragger::TaskCSysDragger(Gui::ViewProviderDragger* vpObjectIn, Gui::SoFCCSysDragger* draggerIn) :
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

void TaskCSysDragger::setupGui()
{
  Gui::TaskView::TaskBox *incrementsBox = new Gui::TaskView::TaskBox(
      Gui::BitmapFactory().pixmap("button_valid"),
      tr("Increments"), true, 0);

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(1, 1);

  QLabel *tLabel = new QLabel(tr("Translation Increment:"), incrementsBox);
  gridLayout->addWidget(tLabel, 0, 0, Qt::AlignRight);

  int spinBoxWidth = QApplication::fontMetrics().averageCharWidth() * 20;
  tSpinBox = new QuantitySpinBox(incrementsBox);
  tSpinBox->setMinimum(0.0);
  tSpinBox->setMaximum(std::numeric_limits<double>::max());
  tSpinBox->setUnit(Base::Unit::Length);
  tSpinBox->setMinimumWidth(spinBoxWidth);
  gridLayout->addWidget(tSpinBox, 0, 1, Qt::AlignLeft);

  QLabel *rLabel = new QLabel(tr("Rotation Increment:"), incrementsBox);
  gridLayout->addWidget(rLabel, 1, 0, Qt::AlignRight);

  rSpinBox = new QuantitySpinBox(incrementsBox);
  rSpinBox->setMinimum(0.0);
  rSpinBox->setMaximum(180.0);
  rSpinBox->setUnit(Base::Unit::Angle);
  rSpinBox->setMinimumWidth(spinBoxWidth);
  gridLayout->addWidget(rSpinBox, 1, 1, Qt::AlignLeft);

  incrementsBox->groupLayout()->addLayout(gridLayout);
  Content.push_back(incrementsBox);

  connect(tSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onTIncrementSlot(double)));
  connect(rSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onRIncrementSlot(double)));
}

void TaskCSysDragger::onTIncrementSlot(double freshValue)
{
  dragger->translationIncrement.setValue(freshValue);
}

void TaskCSysDragger::onRIncrementSlot(double freshValue)
{
  dragger->rotationIncrement.setValue(degreesToRadains(freshValue));
}

void TaskCSysDragger::open()
{
  //we can't have user switching camera types while dragger is shown.
  Gui::Application::Instance->commandManager().getCommandByName("Std_OrthographicCamera")->setEnabled(false);
  Gui::Application::Instance->commandManager().getCommandByName("Std_PerspectiveCamera")->setEnabled(false);
//   dragger->translationIncrement.setValue(lastTranslationIncrement);
//   dragger->rotationIncrement.setValue(lastRotationIncrement);
  tSpinBox->setValue(lastTranslationIncrement);
  rSpinBox->setValue(radiansToDegrees(lastRotationIncrement));

  Gui::TaskView::TaskDialog::open();
}

bool TaskCSysDragger::accept()
{
  lastTranslationIncrement = dragger->translationIncrement.getValue();
  lastRotationIncrement = dragger->rotationIncrement.getValue();

  App::DocumentObject* dObject = vpObject.getObject();
  if (dObject) {
    Gui::Document* document = Gui::Application::Instance->getDocument(dObject->getDocument());
    assert(document);
    document->commitCommand();
    document->resetEdit();
    document->getDocument()->recompute();
  }
  return Gui::TaskView::TaskDialog::accept();
}

#include "moc_TaskCSysDragger.cpp"
