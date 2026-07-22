// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QLocale>
#include <QTest>

#include <App/Application.h>
#include <Base/NumericFormatting.h>

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
    void test_NumericFormattingStateRealignsMismatchedQtAndIcuDefaults()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeEnvironment {
            {.qtLocale = "da_DK", .formattingLocale = "en_US", .icuLocale = "fr_FR"}
        };

        QCOMPARE(QLocale().name(), QStringLiteral("da_DK"));
        QCOMPARE(QString::fromLatin1(icu::Locale::getDefault().getName()), QStringLiteral("fr_FR"));
        QCOMPARE(
            QString::fromStdString(Base::currentNumericFormattingState().localeId),
            QStringLiteral("en_US")
        );

        Gui::Translator::instance()->setLocale("German");
        const auto formattingState = Base::currentNumericFormattingState();

        QCOMPARE(QLocale().name(), QStringLiteral("de_DE"));
        QCOMPARE(QLocale().decimalPoint(), QChar(','));
        QCOMPARE(QLocale().groupSeparator(), QChar('.'));
        QCOMPARE(QString::fromStdString(formattingState.localeId), QStringLiteral("de_DE"));
        QCOMPARE(QString::fromStdString(formattingState.decimalSeparator), QStringLiteral(","));
        QCOMPARE(QString::fromStdString(formattingState.groupingSeparator), QStringLiteral("."));
        QCOMPARE(QString::fromLatin1(icu::Locale::getDefault().getName()), QStringLiteral("de_DE"));
    }

    void test_OperatingSystemFormattingUsesSystemLocaleSeparators()  // NOLINT
    {
        auto general = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/General"
        );
        const int previousPreference = general->GetInt("UseLocaleFormatting", 0);

        general->SetInt(
            "UseLocaleFormatting",
            static_cast<int>(Gui::Translator::LocaleFormattingPreference::OperatingSystem)
        );
        Gui::Translator::instance()->applyLocaleFormattingPreference();

        const auto systemLocale = QLocale::system();
        const auto formatting = Base::currentNumericFormattingState();
        QCOMPARE(
            QString::fromStdString(formatting.decimalSeparator),
            QString(systemLocale.decimalPoint())
        );
        QCOMPARE(
            QString::fromStdString(formatting.groupingSeparator),
            QString(systemLocale.groupSeparator())
        );

        general->SetInt("UseLocaleFormatting", previousPreference);
    }
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testTranslatorLocale)

#include "TranslatorLocale.moc"
