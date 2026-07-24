// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QLocale>

#include <Base/NumericFormatting.h>

namespace Gui
{

/** Return the published numeric locale with separators effective for a widget locale. */
GuiExport Base::NumericFormattingState effectiveNumericFormattingState(const QLocale& locale);

}  // namespace Gui
