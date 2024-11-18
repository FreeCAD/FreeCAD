/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <QWhatsThisClickedEvent>
#endif

#include "WhatsThis.h"
#include "Action.h"
#include "MainWindow.h"


using namespace Gui;


bool StdCmdDescription::_descrMode = false;

/* TRANSLATOR Gui::StdCmdDescription */

StdCmdDescription::StdCmdDescription()
  : Command("Std_DescriptionMode")
{
  sGroup        = "Help";
  sMenuText     = QT_TR_NOOP("Des&cription");
  sToolTipText  = QT_TR_NOOP("Long description of commands");
  sWhatsThis    = "Std_DescriptionMode";
  sStatusTip    = QT_TR_NOOP("Long description of commands");
  sAccel        = "F1";
}

StdCmdDescription::~StdCmdDescription() = default;

Action * StdCmdDescription::createAction()
{
  Action *pcAction = Command::createAction();
  pcAction->setCheckable( true );
  return pcAction;
}

void StdCmdDescription::activated(int iMsg)
{
  Q_UNUSED(iMsg);
  if ( !inDescriptionMode() )
    enterDescriptionMode();
  else
    leaveDescriptionMode();
}

bool StdCmdDescription::inDescriptionMode()
{
  return _descrMode;
}

void StdCmdDescription::setSource( const QString& src )
{
  if ( !src.isEmpty() )
  {
    QWhatsThisClickedEvent e(src);
    QApplication::sendEvent(getMainWindow(), &e);
  }
}

void StdCmdDescription::enterDescriptionMode()
{
  _descrMode = true;
  QApplication::setOverrideCursor(Qt::WhatsThisCursor);
}

void StdCmdDescription::leaveDescriptionMode()
{
  _descrMode = false;
  QApplication::restoreOverrideCursor();
}

