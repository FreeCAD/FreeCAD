// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
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

#include <QFileDialog>

#include <pqWidgetEventPlayer.h>
#include <pqWidgetEventTranslator.h>

#include <Base/Console.h>

#include "QtTestUtility.h"
#include "XMLEventObserver.h"
#include "XMLEventSource.h"

using namespace QtTesting;

TYPESYSTEM_SOURCE(QtTesting::QtTestUtility, Base::BaseClass)

QtTestUtility::QtTestUtility(QObject* parent)
    : pqTestUtility(parent)
{
    addCustomTranslators();
    addCustomEventPlayers();

    addEventObserver(QStringLiteral("xml"), new QtTesting::XMLEventObserver(this));
    addEventSource(QStringLiteral("xml"), new QtTesting::XMLEventSource(this));
}

void QtTestUtility::addWidgetEventTranslator(pqWidgetEventTranslator* translator)
{
    if (translator) {
        eventTranslator()->addWidgetEventTranslator(translator);
    }
}

void QtTestUtility::addCustomTranslators()
{
    // Add any custom translators here
}

void QtTestUtility::addWidgetEventPlayer(pqWidgetEventPlayer* player)
{
    if (player) {
        eventPlayer()->addWidgetEventPlayer(player);
    }
}

void QtTestUtility::addCustomEventPlayers()
{
    // Add any custom event players here
}
