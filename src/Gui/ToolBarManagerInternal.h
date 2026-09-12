// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <list>
#include <string>
#include <utility>

#include <QList>
#include <QString>

#include <FCGlobal.h>

class ParameterManager;

namespace Gui::Internal
{

/** Import toolbar layouts written by the former Tux persistent-toolbar module. */
GuiExport void migrateTuxPersistentToolbars(ParameterManager& parameters);

GuiExport QString findToolbarIdentityCollision(
    const QString& toolbarName,
    const QList<std::list<std::pair<std::string, std::string>>>& workbenchIdentities
);

}  // namespace Gui::Internal
