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

/**
 * Return the format safe for displaying a quantity in an editable field.
 *
 * A dot grouping separator is ambiguous in a locale whose decimal separator is not a dot:
 * `1.234` can be either a grouped integer or a canonical decimal. Editable displays must not
 * emit that ambiguous form.
 */
GuiExport Base::QuantityFormat editableQuantityFormat(
    const Base::QuantityFormat& format,
    const Base::NumericLocaleContext& locale
);

/** Return the UTF-16 length of the numeric token at the start of a line edit. */
GuiExport int numericInputSelectionLength(const QString& text, const Base::NumericLocaleContext& locale);

/** Translate an application-level numeric input diagnostic for GUI presentation. */
GuiExport QString numericInputDiagnosticText(App::InputDiagnosticKind kind);

}  // namespace Gui
