// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>
#include <string_view>

#include <QLocale>
#include <QString>

#include <Base/Tools.h>

#include <unicode/locid.h>
#include <unicode/utypes.h>

namespace tests
{
namespace detail
{
inline icu::Locale toIcuLocale(std::string_view localeId)
{
    if (Base::Tools::isCLocaleName(localeId)) {
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

class ScopedQtDefaultLocale
{
public:
    ScopedQtDefaultLocale()
        : previous {QLocale()}
    {}

    explicit ScopedQtDefaultLocale(const QLocale& locale)
        : previous {QLocale()}
    {
        QLocale::setDefault(locale);
    }

    explicit ScopedQtDefaultLocale(std::string_view localeName)
        : ScopedQtDefaultLocale(detail::toQtLocale(localeName))
    {}

    ~ScopedQtDefaultLocale()
    {
        QLocale::setDefault(previous);
    }

    ScopedQtDefaultLocale(const ScopedQtDefaultLocale&) = delete;
    ScopedQtDefaultLocale(ScopedQtDefaultLocale&&) = delete;
    ScopedQtDefaultLocale& operator=(const ScopedQtDefaultLocale&) = delete;
    ScopedQtDefaultLocale& operator=(ScopedQtDefaultLocale&&) = delete;

private:
    QLocale previous;
};

class ScopedIcuDefaultLocale
{
public:
    ScopedIcuDefaultLocale()
        : previous {icu::Locale::getDefault()}
    {}

    explicit ScopedIcuDefaultLocale(std::string_view localeId)
        : previous {icu::Locale::getDefault()}
    {
        UErrorCode status = U_ZERO_ERROR;
        const icu::Locale locale = detail::toIcuLocale(localeId);
        icu::Locale::setDefault(locale, status);
    }

    ~ScopedIcuDefaultLocale()
    {
        UErrorCode status = U_ZERO_ERROR;
        icu::Locale::setDefault(previous, status);
    }

    ScopedIcuDefaultLocale(const ScopedIcuDefaultLocale&) = delete;
    ScopedIcuDefaultLocale(ScopedIcuDefaultLocale&&) = delete;
    ScopedIcuDefaultLocale& operator=(const ScopedIcuDefaultLocale&) = delete;
    ScopedIcuDefaultLocale& operator=(ScopedIcuDefaultLocale&&) = delete;

private:
    icu::Locale previous;
};

class ScopedCurrentNumericFormattingLocale
{
public:
    ScopedCurrentNumericFormattingLocale()
        : previous {Base::Tools::getCurrentNumericFormattingLocale()}
    {}

    explicit ScopedCurrentNumericFormattingLocale(std::string_view localeName)
        : previous {Base::Tools::getCurrentNumericFormattingLocale()}
    {
        Base::Tools::setCurrentNumericFormattingLocale(localeName);
    }

    ~ScopedCurrentNumericFormattingLocale()
    {
        Base::Tools::setCurrentNumericFormattingLocale(previous);
    }

    ScopedCurrentNumericFormattingLocale(const ScopedCurrentNumericFormattingLocale&) = delete;
    ScopedCurrentNumericFormattingLocale(ScopedCurrentNumericFormattingLocale&&) = delete;
    ScopedCurrentNumericFormattingLocale& operator=(
        const ScopedCurrentNumericFormattingLocale&
    ) = delete;
    ScopedCurrentNumericFormattingLocale& operator=(ScopedCurrentNumericFormattingLocale&&) = delete;

private:
    std::string previous;
};

class ScopedCurrentNumericFormattingSeparators
{
public:
    ScopedCurrentNumericFormattingSeparators()
        : previousDecimal {Base::Tools::getCurrentNumericFormattingDecimalSeparator()}
        , previousGrouping {Base::Tools::getCurrentNumericFormattingGroupingSeparator()}
    {}

    ScopedCurrentNumericFormattingSeparators(
        std::string_view decimalSeparator,
        std::string_view groupingSeparator
    )
        : previousDecimal {Base::Tools::getCurrentNumericFormattingDecimalSeparator()}
        , previousGrouping {Base::Tools::getCurrentNumericFormattingGroupingSeparator()}
    {
        Base::Tools::setCurrentNumericFormattingSeparators(decimalSeparator, groupingSeparator);
    }

    explicit ScopedCurrentNumericFormattingSeparators(const QLocale& locale)
        : ScopedCurrentNumericFormattingSeparators(
              detail::toUtf8(QString(locale.decimalPoint())),
              detail::toUtf8(QString(locale.groupSeparator()))
          )
    {}

    ~ScopedCurrentNumericFormattingSeparators()
    {
        Base::Tools::setCurrentNumericFormattingSeparators(previousDecimal, previousGrouping);
    }

    ScopedCurrentNumericFormattingSeparators(const ScopedCurrentNumericFormattingSeparators&) = delete;
    ScopedCurrentNumericFormattingSeparators(ScopedCurrentNumericFormattingSeparators&&) = delete;
    ScopedCurrentNumericFormattingSeparators& operator=(
        const ScopedCurrentNumericFormattingSeparators&
    ) = delete;
    ScopedCurrentNumericFormattingSeparators& operator=(
        ScopedCurrentNumericFormattingSeparators&&
    ) = delete;

private:
    std::string previousDecimal;
    std::string previousGrouping;
};

class ScopedOperatingSystemNumericLocale
{
public:
    ScopedOperatingSystemNumericLocale()
        : previous {Base::Tools::getOperatingSystemNumericLocale()}
    {}

    explicit ScopedOperatingSystemNumericLocale(std::string_view localeName)
        : previous {Base::Tools::getOperatingSystemNumericLocale()}
    {
        Base::Tools::setOperatingSystemNumericLocale(localeName);
    }

    ~ScopedOperatingSystemNumericLocale()
    {
        Base::Tools::setOperatingSystemNumericLocale(previous);
    }

    ScopedOperatingSystemNumericLocale(const ScopedOperatingSystemNumericLocale&) = delete;
    ScopedOperatingSystemNumericLocale(ScopedOperatingSystemNumericLocale&&) = delete;
    ScopedOperatingSystemNumericLocale& operator=(const ScopedOperatingSystemNumericLocale&) = delete;
    ScopedOperatingSystemNumericLocale& operator=(ScopedOperatingSystemNumericLocale&&) = delete;

private:
    std::string previous;
};

class ScopedFormattingLocaleState
{
public:
    ScopedFormattingLocaleState(std::string_view formattingLocale, std::string_view icuLocale)
        : formatting {formattingLocale}
        , icu {icuLocale}
    {}

    ScopedFormattingLocaleState(const ScopedFormattingLocaleState&) = delete;
    ScopedFormattingLocaleState(ScopedFormattingLocaleState&&) = delete;
    ScopedFormattingLocaleState& operator=(const ScopedFormattingLocaleState&) = delete;
    ScopedFormattingLocaleState& operator=(ScopedFormattingLocaleState&&) = delete;

private:
    ScopedCurrentNumericFormattingLocale formatting;
    ScopedIcuDefaultLocale icu;
};

class ScopedNumericLocaleState
{
public:
    ScopedNumericLocaleState(
        std::string_view qtLocale,
        std::string_view formattingLocale,
        std::string_view icuLocale
    )
        : qt {qtLocale}
        , formatting {formattingLocale}
        , separators {detail::toQtLocale(qtLocale)}
        , icu {icuLocale}
    {}

    ScopedNumericLocaleState(const ScopedNumericLocaleState&) = delete;
    ScopedNumericLocaleState(ScopedNumericLocaleState&&) = delete;
    ScopedNumericLocaleState& operator=(const ScopedNumericLocaleState&) = delete;
    ScopedNumericLocaleState& operator=(ScopedNumericLocaleState&&) = delete;

private:
    ScopedQtDefaultLocale qt;
    ScopedCurrentNumericFormattingLocale formatting;
    ScopedCurrentNumericFormattingSeparators separators;
    ScopedIcuDefaultLocale icu;
};

}  // namespace tests
