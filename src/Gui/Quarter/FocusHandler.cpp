/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SIM::Coin3D::Quarter::FocusHandler FocusHandler.h Quarter/devices/FocusHandler.h

  \brief The FocusHandler eventfilter provides Coin with focus in and
  focus out events, if installed on QuarterWidget.
*/

#include <QEvent>
#include <Inventor/SoEventManager.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>

#include "QuarterWidget.h"
#include "eventhandlers/FocusHandler.h"


using namespace SIM::Coin3D::Quarter;

FocusHandler::FocusHandler(QObject * parent)
  : QObject(parent)
{
  this->quarterwidget = dynamic_cast<QuarterWidget *>(parent);
}

FocusHandler::~FocusHandler()
{

}

bool 
FocusHandler::eventFilter(QObject * obj, QEvent * event)
{
  switch (event->type()) {
  case QEvent::FocusIn:
    this->focusEvent("sim.coin3d.coin.InputFocus.IN");
    break;
  case QEvent::FocusOut:
    this->focusEvent("sim.coin3d.coin.InputFocus.OUT");
    break;
  default:
    break;
  }
  return QObject::eventFilter(obj, event);
}

void
FocusHandler::focusEvent(const SbName & focusevent)
{
  SoEventManager * eventmanager = this->quarterwidget->getSoEventManager();
  for (int c = 0; c < eventmanager->getNumSoScXMLStateMachines(); ++c) {
    SoScXMLStateMachine * sostatemachine =
      eventmanager->getSoScXMLStateMachine(c);
    if (sostatemachine->isActive()) {
      sostatemachine->queueEvent(focusevent);
      sostatemachine->processEventQueue();
    }
  }
}
