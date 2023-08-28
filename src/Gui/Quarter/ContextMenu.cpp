
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

#include <QActionGroup>
#include <QMenu>

#include "ContextMenu.h"
#include "QuarterWidget.h"


using namespace SIM::Coin3D::Quarter;

ContextMenu::ContextMenu(QuarterWidget * quarterwidget)
  : quarterwidget(quarterwidget)
{
  this->contextmenu = new QMenu;
  this->functionsmenu = new QMenu("Functions");
  this->rendermenu = new QMenu("Render Mode");
  this->stereomenu = new QMenu("Stereo Mode");
  this->transparencymenu = new QMenu("Transparency Type");

  this->contextmenu->addMenu(functionsmenu);
  this->contextmenu->addMenu(rendermenu);
  this->contextmenu->addMenu(stereomenu);
  this->contextmenu->addMenu(transparencymenu);

  SoRenderManager * sorendermanager = quarterwidget->getSoRenderManager();

  QActionGroup * rendermodegroup = nullptr;
  QActionGroup * stereomodegroup = nullptr;
  QActionGroup * transparencytypegroup = nullptr;

  Q_FOREACH (QAction * action, quarterwidget->renderModeActions()) {
    if (!rendermodegroup) {
      rendermodegroup = action->actionGroup();
    } else {
      assert(rendermodegroup && rendermodegroup == action->actionGroup());
    }

    int rendermode = static_cast<QuarterWidget::RenderMode>(sorendermanager->getRenderMode());
    int data = static_cast<QuarterWidget::RenderMode>(action->data().toInt());
    action->setChecked(rendermode == data);
    rendermenu->addAction(action);
  }

  Q_FOREACH (QAction * action, quarterwidget->stereoModeActions()) {
    if (!stereomodegroup) {
      stereomodegroup = action->actionGroup();
    } else {
      assert(stereomodegroup && stereomodegroup == action->actionGroup());
    }

    int stereomode = static_cast<QuarterWidget::StereoMode>(sorendermanager->getStereoMode());
    int data = static_cast<QuarterWidget::StereoMode>(action->data().toInt());
    action->setChecked(stereomode == data);
    stereomenu->addAction(action);
  }

  Q_FOREACH (QAction * action, quarterwidget->transparencyTypeActions()) {
    if (!transparencytypegroup) {
      transparencytypegroup = action->actionGroup();
    } else {
      assert(transparencytypegroup && transparencytypegroup == action->actionGroup());
    }

    SoGLRenderAction * renderaction = sorendermanager->getGLRenderAction();
    int transparencytype = static_cast<SoGLRenderAction::TransparencyType>(renderaction->getTransparencyType());
    int data = static_cast<SoGLRenderAction::TransparencyType>(action->data().toInt());
    action->setChecked(transparencytype == data);
    transparencymenu->addAction(action);
  }

  QAction * viewall = new QAction("View All", quarterwidget);
  QAction * seek = new QAction("Seek", quarterwidget);
  functionsmenu->addAction(viewall);
  functionsmenu->addAction(seek);

  QObject::connect(seek, &QAction::triggered,
                   this->quarterwidget, &QuarterWidget::seek);

  QObject::connect(viewall, &QAction::triggered,
                   this->quarterwidget, &QuarterWidget::viewAll);

  // FIXME: It would be ideal to expose these actiongroups to Qt
  // Designer and be able to connect them to the appropriate slots on
  // QuarterWidget, but this is not possible in Qt. Exposing every
  // single action is supposed to work, but it doesn't at the
  // moment. (20081215 frodo)
  QObject::connect(rendermodegroup, &QActionGroup::triggered,
                   this, &ContextMenu::changeRenderMode);

  QObject::connect(stereomodegroup, &QActionGroup::triggered,
                   this, &ContextMenu::changeStereoMode);

  QObject::connect(transparencytypegroup, &QActionGroup::triggered,
                   this, &ContextMenu::changeTransparencyType);
}

ContextMenu::~ContextMenu()
{
  delete this->functionsmenu;
  delete this->rendermenu;
  delete this->stereomenu;
  delete this->transparencymenu;
  delete this->contextmenu;
}

QMenu *
ContextMenu::getMenu() const
{
  return this->contextmenu;
}

void
ContextMenu::changeRenderMode(QAction * action)
{
  QuarterWidget::RenderMode mode =
    static_cast<QuarterWidget::RenderMode>(action->data().toInt());

  this->quarterwidget->setRenderMode(mode);
  this->quarterwidget->getSoRenderManager()->scheduleRedraw();
}

void
ContextMenu::changeStereoMode(QAction * action)
{
  QuarterWidget::StereoMode mode =
    static_cast<QuarterWidget::StereoMode>(action->data().toInt());

  this->quarterwidget->setStereoMode(mode);
  this->quarterwidget->getSoRenderManager()->scheduleRedraw();
}

void
ContextMenu::changeTransparencyType(QAction * action)
{
  QuarterWidget::TransparencyType type =
    static_cast<QuarterWidget::TransparencyType>(action->data().toInt());

  this->quarterwidget->setTransparencyType(type);
  this->quarterwidget->getSoRenderManager()->scheduleRedraw();
}
