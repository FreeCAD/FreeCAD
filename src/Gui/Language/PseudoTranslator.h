// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QTranslator>

namespace Gui
{
class PseudoTranslator final: public QTranslator
{
    bool isEmpty() const override;
    QString translate(
        const char* context,
        const char* sourceText,
        const char* disambiguation,
        int n
    ) const override;
};
}  // namespace Gui
