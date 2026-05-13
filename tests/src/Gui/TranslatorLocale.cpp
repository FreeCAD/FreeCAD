// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QLocale>
#include <QTest>

#include <Base/Tools.h>

#include "Gui/Language/Translator.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

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
        tests::ScopedQtDefaultLocale qtLocale;
        tests::ScopedIcuDefaultLocale icuLocale;
        tests::ScopedCurrentNumericFormattingLocale formattingLocale;
        tests::ScopedOperatingSystemNumericLocale operatingSystemNumericLocale {"de_DE.UTF-8"};

        Gui::Translator::instance()->setLocale();

        QCOMPARE(QLocale().name(), QStringLiteral("de_DE"));
        QCOMPARE(QLocale().decimalPoint(), QChar(','));
        QCOMPARE(QLocale().groupSeparator(), QChar('.'));
        QCOMPARE(
            QString::fromStdString(Base::Tools::getCurrentNumericFormattingLocale()),
            QStringLiteral("de_DE")
        );
        QCOMPARE(
            QString::fromStdString(Base::Tools::getCurrentNumericFormattingDecimalSeparator()),
            QStringLiteral(",")
        );
        QCOMPARE(
            QString::fromStdString(Base::Tools::getCurrentNumericFormattingGroupingSeparator()),
            QStringLiteral(".")
        );
        QCOMPARE(QString::fromLatin1(icu::Locale::getDefault().getName()), QStringLiteral("de_DE"));
    }
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testTranslatorLocale)

#include "TranslatorLocale.moc"
