#ifndef QUARTER_INTERACTIONMODE_H
#define QUARTER_INTERACTIONMODE_H

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

#include <QCursor>
#include <QObject>
#include <Inventor/SoEventManager.h>

#include "Basic.h"


class QEvent;
class SoEvent;
class QKeyEvent;
class QFocusEvent;

namespace SIM { namespace Coin3D { namespace Quarter {

class QuarterWidget;

class QUARTER_DLL_API InteractionMode : public QObject {
  Q_OBJECT
public:
  InteractionMode(QuarterWidget * quarterwidget);
  ~InteractionMode() override;

  void setEnabled(bool yes);
  bool enabled() const;

  void setOn(bool on);
  bool on() const;

protected:
  bool eventFilter(QObject *, QEvent * event) override;

private:
  bool keyPressEvent(QKeyEvent * event);
  bool keyReleaseEvent(QKeyEvent * event);
  bool focusOutEvent(QFocusEvent * event);

  QCursor prevcursor;
  QuarterWidget * quarterwidget;
  bool altkeydown;
  SoEventManager::NavigationState prevnavstate;
  bool isenabled;
};

}}} // namespace

#endif // QUARTER_INTERACTIONMODE_H
