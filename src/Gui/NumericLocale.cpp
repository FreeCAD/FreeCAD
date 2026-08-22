// SPDX-License-Identifier: LGPL-2.1-or-later

#include "NumericLocale.h"

#include <utility>

#include <QCoreApplication>
#include <QStringList>

#include <App/QuantityInput.h>
#include <Base/NumericFormatting.h>
#include <Base/NumericInput.h>
#include <Base/Quantity.h>

namespace
{
std::string toUtf8(const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    return std::string(utf8.constData(), utf8.size());
}

std::pair<int, int> groupingSizes(const QLocale& locale)
{
    const QString separator = locale.groupSeparator();
    const QString formatted = locale.toString(123456789.0, 'f', 0);
    const auto groups = formatted.split(separator, Qt::KeepEmptyParts);
    if (separator.isEmpty() || groups.size() < 2) {
        return {0, 0};
    }

    const int primary = groups.back().size();
    const int secondary = groups.size() > 2 ? groups[groups.size() - 2].size() : primary;
    return {primary, secondary};
}
}  // namespace

Base::NumericLocaleContext Gui::numericLocaleContextFor(const QLocale& locale)
{
    const auto [primary, secondary] = groupingSizes(locale);
    const QString localeName = locale.name();

    return {
        Base::normalizeIcuLocaleId(toUtf8(localeName)),
        toUtf8(QString(locale.decimalPoint())),
        toUtf8(QString(locale.groupSeparator())),
        toUtf8(QString(locale.positiveSign())),
        toUtf8(QString(locale.negativeSign())),
        primary,
        secondary,
        toUtf8(QString(locale.zeroDigit()))
    };
}

Base::QuantityFormat Gui::editableQuantityFormat(
    const Base::QuantityFormat& format,
    const Base::NumericLocaleContext& locale
)
{
    auto editable = format;
    if (locale.decimalSeparator != "." && locale.groupingSeparator == ".") {
        editable.option |= Base::QuantityFormat::OmitGroupSeparator;
    }
    return editable;
}

int Gui::numericInputSelectionLength(const QString& text, const Base::NumericLocaleContext& locale)
{
    const QByteArray utf8 = text.toUtf8();
    const auto result = Base::scanLocalizedNumber(
        std::string_view {utf8.constData(), static_cast<std::size_t>(utf8.size())},
        locale,
        Base::NumericSyntaxContext::Standalone
    );
    if (result.consumedBytes == 0) {
        return 0;
    }

    return QString::fromUtf8(utf8.constData(), static_cast<int>(result.consumedBytes)).size();
}

QString Gui::numericInputDiagnosticText(const App::InputDiagnosticKind kind)
{
    switch (kind) {
        case App::InputDiagnosticKind::IncompleteNumber:
            return QCoreApplication::translate("NumericInput", "Incomplete number");
        case App::InputDiagnosticKind::MalformedGrouping:
            return QCoreApplication::translate("NumericInput", "Malformed grouping separator placement");
        case App::InputDiagnosticKind::InvalidNumber:
            return QCoreApplication::translate("NumericInput", "Invalid number");
        case App::InputDiagnosticKind::ExpressionSyntax:
            return QCoreApplication::translate("NumericInput", "Invalid expression");
        case App::InputDiagnosticKind::Evaluation:
            return QCoreApplication::translate("NumericInput", "Expression could not be evaluated");
        case App::InputDiagnosticKind::IncompatibleUnit:
            return QCoreApplication::translate("NumericInput", "Incompatible unit");
        case App::InputDiagnosticKind::OutOfRange:
            return QCoreApplication::translate("NumericInput", "Value is outside the allowed range");
    }
    return {};
}
