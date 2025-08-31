// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QAction>
#include <QMenu>
#include <vector>
#include <sstream>
#include <iostream>
#endif

#include <App/Link.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>

#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>

#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyLink.h>

#include "ViewProviderAssembly.h"
#include "ViewProviderAssemblyLink.h"


using namespace Assembly;
using namespace AssemblyGui;


PROPERTY_SOURCE(AssemblyGui::ViewProviderAssemblyLink, Gui::ViewProviderPart)

ViewProviderAssemblyLink::ViewProviderAssemblyLink()
{}

ViewProviderAssemblyLink::~ViewProviderAssemblyLink() = default;

QIcon ViewProviderAssemblyLink::getIcon() const
{
    auto* assembly = dynamic_cast<Assembly::AssemblyLink*>(getObject());
    if (assembly->isRigid()) {
        return Gui::BitmapFactory().pixmap("Assembly_AssemblyLinkRigid.svg");
    }
    else {
        return Gui::BitmapFactory().pixmap("Assembly_AssemblyLink.svg");
    }
}

bool ViewProviderAssemblyLink::setEdit(int mode)
{
    auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());

    if (!assemblyLink->isRigid() && mode == (int)ViewProvider::Transform) {
        Base::Console().userTranslatedNotification(
            "Flexible sub-assemblies cannot be transformed.");
        return true;
    }

    return ViewProviderPart::setEdit(mode);
}

bool ViewProviderAssemblyLink::doubleClicked()
{
    auto* link = freecad_cast<AssemblyLink*>(getObject());

    if (!link) {
        return true;
    }
    auto* assembly = link->getLinkedAssembly();

    auto* vpa =
        freecad_cast<ViewProviderAssembly*>(Gui::Application::Instance->getViewProvider(assembly));
    if (!vpa) {
        return true;
    }

    return vpa->doubleClicked();
}

bool ViewProviderAssemblyLink::onDelete(const std::vector<std::string>& subNames)
{
    Q_UNUSED(subNames)

    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()",
                            getObject()->getDocument()->getName(),
                            getObject()->getNameInDocument());

    // getObject()->purgeTouched();

    return ViewProviderPart::onDelete(subNames);
}

void ViewProviderAssemblyLink::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto func = new Gui::ActionFunction(menu);
    QAction* act;
    auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
    if (assemblyLink->isRigid()) {
        act = menu->addAction(QObject::tr("Turn flexible"));
        act->setToolTip(QObject::tr(
            "Your sub-assembly is currently rigid. This will make it flexible instead."));
    }
    else {
        act = menu->addAction(QObject::tr("Turn rigid"));
        act->setToolTip(QObject::tr(
            "Your sub-assembly is currently flexible. This will make it rigid instead."));
    }

    func->trigger(act, [this]() {
        auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Toggle Rigid"));
        Gui::cmdAppObjectArgs(assemblyLink,
                              "Rigid = %s",
                              assemblyLink->Rigid.getValue() ? "False" : "True");

        Gui::Command::commitCommand();
        Gui::Selection().clearSelection();
    });

    Q_UNUSED(receiver)
    Q_UNUSED(member)
}
