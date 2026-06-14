// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <Gui/DeferredDialogRejectUtils.h>

namespace PartDesignGui
{
using DeferredDialogRejectState = Gui::DeferredDialogRejectState;
using Gui::ensureDeferredDialogRejectConnection;
using Gui::finishDeferredDialogReject;
using Gui::setDeferredDialogRejectPending;

}  // namespace PartDesignGui
