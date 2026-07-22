// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NumericLocale.h"

Base::NumericFormattingState Gui::effectiveNumericFormattingState(const QLocale& locale)
{
    auto formatting = Base::currentNumericFormattingState();
    formatting.decimalSeparator = locale.decimalPoint().toUtf8().toStdString();
    formatting.groupingSeparator = locale.groupSeparator().toUtf8().toStdString();
    return formatting;
}
