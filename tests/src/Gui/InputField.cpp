// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QTest>

#include "Gui/InputField.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

class testInputField: public QObject
{
    Q_OBJECT

public:
    testInputField()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void test_MismatchedFormatterAndWidgetLocaleDoesNotMutateValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        input.setValue(Base::Quantity(10, "mm"));

        QCOMPARE(input.getQuantity(), Base::Quantity(10, "mm"));
        QCOMPARE(input.rawValue(), 10.0);
    }

    void test_ValidatorUsesOriginalTextBeforeLocaleFixup()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        input.setMaximum(500);

        QString text = QStringLiteral("10.00 mm");
        int pos = 0;

        QCOMPARE(input.validate(text, pos), QValidator::Acceptable);
    }

    void test_GroupedLocaleNumberIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        input.setText(QStringLiteral("1,010.00 mm"));

        QCOMPARE(input.getQuantity(), Base::Quantity(1010, "mm"));
        QCOMPARE(input.rawValue(), 1010.0);
    }
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testInputField)

#include "InputField.moc"
