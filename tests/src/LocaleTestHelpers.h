// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <clocale>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#if (defined(FREECAD_TEST_HAS_QT) && FREECAD_TEST_HAS_QT) || defined(QT_CORE_LIB)
# define FREECAD_TEST_LOCALE_HAS_QT 1
# include <QLocale>
# include <QString>
#else
# define FREECAD_TEST_LOCALE_HAS_QT 0
#endif

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

#if FREECAD_TEST_LOCALE_HAS_QT
inline QLocale toQtLocale(std::string_view localeName)
{
    return QLocale(QString::fromUtf8(localeName.data(), static_cast<int>(localeName.size())));
}

inline std::string toUtf8(const QString& text)
{
    const QByteArray utf8 = text.toUtf8();
    return std::string(utf8.constData(), utf8.size());
}
#endif
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
#if FREECAD_TEST_LOCALE_HAS_QT
    std::optional<std::string_view> qtLocale {};
#endif
    std::optional<std::string_view> formattingLocale {};
    std::optional<std::string_view> icuLocale {};
#if FREECAD_TEST_LOCALE_HAS_QT
    bool useQtSeparators {false};
#endif
};

class ScopedLocaleEnvironment
{
public:
    explicit ScopedLocaleEnvironment(const LocaleEnvironmentConfig& config = {})
        : previousIcu {icu::Locale::getDefault()}
        , previousFormatting {Base::currentNumericLocaleContext()}
        , previousCNumeric {currentCNumericLocale()}
    {
        std::setlocale(LC_NUMERIC, "C");

#if FREECAD_TEST_LOCALE_HAS_QT
        std::optional<QLocale> qtLocale;
        if (config.qtLocale) {
            qtLocale = detail::toQtLocale(*config.qtLocale);
        }
        QLocale::setDefault(qtLocale.value_or(QLocale::c()));
#endif

        if (config.icuLocale) {
            UErrorCode status = U_ZERO_ERROR;
            icu::Locale::setDefault(detail::toIcuLocale(*config.icuLocale), status);
        }

        if (
            config.formattingLocale
#if FREECAD_TEST_LOCALE_HAS_QT
            || config.useQtSeparators
#endif
        ) {
            auto formatting = config.formattingLocale
                ? Base::createNumericLocaleContext(*config.formattingLocale)
                : previousFormatting;
#if FREECAD_TEST_LOCALE_HAS_QT
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
#endif
            Base::publishNumericLocaleContext(std::move(formatting));
        }
    }

    ~ScopedLocaleEnvironment()
    {
        std::setlocale(LC_NUMERIC, previousCNumeric.c_str());

        Base::publishNumericLocaleContext(previousFormatting);

        UErrorCode status = U_ZERO_ERROR;
        icu::Locale::setDefault(previousIcu, status);
#if FREECAD_TEST_LOCALE_HAS_QT
        QLocale::setDefault(previousQt);
#endif
    }

    ScopedLocaleEnvironment(const ScopedLocaleEnvironment&) = delete;
    ScopedLocaleEnvironment(ScopedLocaleEnvironment&&) = delete;
    ScopedLocaleEnvironment& operator=(const ScopedLocaleEnvironment&) = delete;
    ScopedLocaleEnvironment& operator=(ScopedLocaleEnvironment&&) = delete;

private:
    static std::string currentCNumericLocale()
    {
        const char* name = std::setlocale(LC_NUMERIC, nullptr);
        return name ? std::string(name) : std::string("C");
    }

#if FREECAD_TEST_LOCALE_HAS_QT
    QLocale previousQt {QLocale()};
#endif
    icu::Locale previousIcu;
    Base::NumericLocaleContext previousFormatting;
    std::string previousCNumeric;
};

}  // namespace tests

#undef FREECAD_TEST_LOCALE_HAS_QT
