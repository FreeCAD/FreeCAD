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

class ScopedNumericLocaleContext
{
public:
    explicit ScopedNumericLocaleContext(Base::NumericLocaleContext next)
        : previous {Base::currentNumericLocaleContext()}
    {
        Base::publishNumericLocaleContext(std::move(next));
    }

    ~ScopedNumericLocaleContext()
    {
        Base::publishNumericLocaleContext(previous);
    }

    ScopedNumericLocaleContext(const ScopedNumericLocaleContext&) = delete;
    ScopedNumericLocaleContext(ScopedNumericLocaleContext&&) = delete;
    ScopedNumericLocaleContext& operator=(const ScopedNumericLocaleContext&) = delete;
    ScopedNumericLocaleContext& operator=(ScopedNumericLocaleContext&&) = delete;

private:
    Base::NumericLocaleContext previous;
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
        , previousFormatting {Base::currentNumericLocaleContext()}
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
                ? Base::createNumericLocaleContext(*config.formattingLocale)
                : previousFormatting;
            if (config.useQtSeparators) {
                const QLocale& locale = qtLocale ? *qtLocale : QLocale();
                formatting.decimalSeparator = detail::toUtf8(QString(locale.decimalPoint()));
                formatting.groupingSeparator = detail::toUtf8(QString(locale.groupSeparator()));
                formatting.positiveSign = detail::toUtf8(QString(locale.positiveSign()));
                formatting.negativeSign = detail::toUtf8(QString(locale.negativeSign()));
                formatting.zeroDigit = detail::toUtf8(QString(locale.zeroDigit()));
                const auto groups = locale.toString(123456789.0, 'f', 0)
                                        .split(locale.groupSeparator(), Qt::KeepEmptyParts);
                if (groups.size() > 1) {
                    formatting.primaryGroupingSize = groups.back().size();
                    formatting.secondaryGroupingSize = groups.size() > 2
                        ? groups[groups.size() - 2].size()
                        : formatting.primaryGroupingSize;
                }
            }
            Base::publishNumericLocaleContext(std::move(formatting));
        }
    }

    ~ScopedLocaleEnvironment()
    {
        Base::publishNumericLocaleContext(previousFormatting);

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
    Base::NumericLocaleContext previousFormatting;
};

}  // namespace tests
