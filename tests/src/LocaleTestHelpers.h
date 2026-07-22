// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <QLocale>
#include <QString>

#include <Base/NumericFormatting.h>

#include <unicode/locid.h>
#include <unicode/utypes.h>

namespace tests
{
namespace detail
{
inline icu::Locale toIcuLocale(std::string_view localeId)
{
    if (Base::isCLocaleName(localeId)) {
        return icu::Locale("en_US_POSIX");
    }

    const std::string localeName(localeId);
    return icu::Locale::createFromName(localeName.c_str());
}

inline QLocale toQtLocale(std::string_view localeName)
{
    return QLocale(QString::fromUtf8(localeName.data(), static_cast<int>(localeName.size())));
}

inline std::string toUtf8(const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    return std::string(utf8.constData(), utf8.size());
}
}  // namespace detail

class ScopedNumericFormattingState
{
public:
    ScopedNumericFormattingState(std::string_view decimalSeparator, std::string_view groupingSeparator)
        : previous {Base::currentNumericFormattingState()}
    {
        auto next = previous;
        next.decimalSeparator = decimalSeparator;
        next.groupingSeparator = groupingSeparator;
        Base::publishNumericFormattingState(std::move(next));
    }

    ~ScopedNumericFormattingState()
    {
        Base::publishNumericFormattingState(previous);
    }

    ScopedNumericFormattingState(const ScopedNumericFormattingState&) = delete;
    ScopedNumericFormattingState(ScopedNumericFormattingState&&) = delete;
    ScopedNumericFormattingState& operator=(const ScopedNumericFormattingState&) = delete;
    ScopedNumericFormattingState& operator=(ScopedNumericFormattingState&&) = delete;

private:
    Base::NumericFormattingState previous;
};

struct LocaleEnvironmentConfig
{
    std::optional<std::string_view> qtLocale {};
    std::optional<std::string_view> formattingLocale {};
    std::optional<std::string_view> icuLocale {};
    bool useQtSeparators {false};
};

class ScopedLocaleEnvironment
{
public:
    explicit ScopedLocaleEnvironment(const LocaleEnvironmentConfig& config = {})
        : previousQt {QLocale()}
        , previousIcu {icu::Locale::getDefault()}
        , previousFormatting {Base::currentNumericFormattingState()}
    {
        std::optional<QLocale> qtLocale;
        if (config.qtLocale) {
            qtLocale = detail::toQtLocale(*config.qtLocale);
            QLocale::setDefault(*qtLocale);
        }

        if (config.icuLocale) {
            UErrorCode status = U_ZERO_ERROR;
            icu::Locale::setDefault(detail::toIcuLocale(*config.icuLocale), status);
        }

        if (config.formattingLocale || config.useQtSeparators) {
            auto formatting = config.formattingLocale
                ? Base::createNumericFormattingState(*config.formattingLocale)
                : previousFormatting;
            if (config.useQtSeparators) {
                const QLocale& locale = qtLocale ? *qtLocale : QLocale();
                formatting.decimalSeparator = detail::toUtf8(QString(locale.decimalPoint()));
                formatting.groupingSeparator = detail::toUtf8(QString(locale.groupSeparator()));
            }
            Base::publishNumericFormattingState(std::move(formatting));
        }
    }

    ~ScopedLocaleEnvironment()
    {
        Base::publishNumericFormattingState(previousFormatting);

        UErrorCode status = U_ZERO_ERROR;
        icu::Locale::setDefault(previousIcu, status);
        QLocale::setDefault(previousQt);
    }

    ScopedLocaleEnvironment(const ScopedLocaleEnvironment&) = delete;
    ScopedLocaleEnvironment(ScopedLocaleEnvironment&&) = delete;
    ScopedLocaleEnvironment& operator=(const ScopedLocaleEnvironment&) = delete;
    ScopedLocaleEnvironment& operator=(ScopedLocaleEnvironment&&) = delete;

private:
    QLocale previousQt;
    icu::Locale previousIcu;
    Base::NumericFormattingState previousFormatting;
};

}  // namespace tests
