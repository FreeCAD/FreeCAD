// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QLocale>
#include <QString>

#include <Base/NumericFormatting.h>

namespace App
{
enum class InputDiagnosticKind;
}

namespace Gui
{

/** Build a complete numeric-locale context from this widget's QLocale. */
GuiExport Base::NumericLocaleContext numericLocaleContextFor(const QLocale& locale);

/** Return the UTF-16 code-unit length of the numeric token at the start of a line edit. */
GuiExport int numericInputSelectionLengthUtf16(
    const QString& text,
    const Base::NumericLocaleContext& locale
);

/** Translate an application-level numeric input diagnostic for GUI presentation. */
GuiExport QString numericInputDiagnosticText(App::InputDiagnosticKind kind);

}  // namespace Gui
