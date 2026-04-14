// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QLocale>
#include <QTest>

#include <Base/Tools.h>

#include <unicode/locid.h>
#include <unicode/utypes.h>

#include "Gui/Language/Translator.h"
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
class ScopedLocaleState
{
public:
    explicit ScopedLocaleState(const char* operatingSystemNumericLocale)
        : previousQtLocale {QLocale()}
        , previousIcuLocale {icu::Locale::getDefault()}
        , previousOperatingSystemNumericLocale {Base::Tools::getOperatingSystemNumericLocale()}
    {
        Base::Tools::setOperatingSystemNumericLocale(operatingSystemNumericLocale);
    }

    ~ScopedLocaleState()
    {
        QLocale::setDefault(previousQtLocale);

        UErrorCode status = U_ZERO_ERROR;
        icu::Locale::setDefault(previousIcuLocale, status);

        Base::Tools::setOperatingSystemNumericLocale(previousOperatingSystemNumericLocale);
    }

    ScopedLocaleState(const ScopedLocaleState&) = delete;
    ScopedLocaleState& operator=(const ScopedLocaleState&) = delete;

private:
    QLocale previousQtLocale;
    icu::Locale previousIcuLocale;
    std::string previousOperatingSystemNumericLocale;
};
}  // namespace

class testTranslatorLocale: public QObject
{
    Q_OBJECT

public:
    testTranslatorLocale()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void test_OperatingSystemNumericLocaleKeepsQtAndIcuAligned()  // NOLINT
    {
        ScopedLocaleState localeState("de_DE.UTF-8");

        Gui::Translator::instance()->setLocale();

        QCOMPARE(QLocale().name(), QStringLiteral("de_DE"));
        QCOMPARE(QLocale().decimalPoint(), QChar(','));
        QCOMPARE(QLocale().groupSeparator(), QChar('.'));
        QCOMPARE(QString::fromLatin1(icu::Locale::getDefault().getName()), QStringLiteral("de_DE"));
    }
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testTranslatorLocale)

#include "TranslatorLocale.moc"
