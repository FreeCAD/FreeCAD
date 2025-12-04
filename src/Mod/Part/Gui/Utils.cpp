// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Walter Steffè  <walter.steffe.it@gmail.com>         *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "FCGlobal.h"

#include <QMessageBox>
#include <QString>
#include <App/GeoFeature.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/MDIView.h>
#include <Gui/ActiveObjectList.h>

#include "Utils.h"


FC_LOG_LEVEL_INIT("PartGui", true, true)


//===========================================================================
// Helper for Body
//===========================================================================

namespace PartGui
{

QString getAutoGroupCommandStr(bool useActiveBody)
// Helper function to get the python code to add the newly created object to the active Part/Body
// object if present
{
    App::GeoFeature* activeObj = nullptr;
    if (useActiveBody) {
        activeObj = Gui::Application::Instance->activeView()->getActiveObject<App::GeoFeature*>(
            PDBODYKEY
        );
    }
    if (!activeObj) {
        activeObj = Gui::Application::Instance->activeView()->getActiveObject<App::GeoFeature*>(
            PARTKEY
        );
    }
    if (activeObj) {
        QString activeName = QString::fromLatin1(activeObj->getNameInDocument());
        return QStringLiteral("App.ActiveDocument.getObject('%1\').addObject(obj)\n").arg(activeName);
    }
    return QStringLiteral("# Object created at document root.\n");
}

QString getAutoGroupCommandStr(QString objectName)
// Helper function to get the python code to add the newly created object to the active Part object
// if present
{
    App::Part* activePart = Gui::Application::Instance->activeView()->getActiveObject<App::Part*>(
        "part"
    );
    if (activePart) {
        QString activeObjectName = QString::fromLatin1(activePart->getNameInDocument());
        return QStringLiteral(
                   "App.ActiveDocument.getObject('%1\')."
                   "addObject(App.ActiveDocument.getObject('%2\'))\n"
        )
            .arg(activeObjectName, objectName);
    }
    return QStringLiteral("# Object %1 created at document root").arg(objectName);
}


}  // namespace PartGui
