// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <list>
#include <string>
#include <utility>

#include <QList>
#include <QString>

#include <FCGlobal.h>

namespace Gui::Internal
{

GuiExport QString findToolbarIdentityCollision(
    const QString& toolbarName,
    const QList<std::list<std::pair<std::string, std::string>>>& workbenchIdentities
);

}  // namespace Gui::Internal
