// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QLocale>
#include <QTest>

#include <Base/Tools.h>

#include <unicode/locid.h>
#include <unicode/utypes.h>

#include "Gui/InputField.h"
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
class ScopedNumericLocales
{
public:
    ScopedNumericLocales(const char* qtLocale, const char* icuLocale)
        : previousQtLocale {QLocale()}
        , previousIcuLocale {icu::Locale::getDefault()}
    {
        QLocale::setDefault(QLocale(QString::fromLatin1(qtLocale)));
        Base::Tools::setIcuDefaultLocale(icuLocale);
    }

    ~ScopedNumericLocales()
    {
        QLocale::setDefault(previousQtLocale);

        UErrorCode status = U_ZERO_ERROR;
        icu::Locale::setDefault(previousIcuLocale, status);
    }

    ScopedNumericLocales(const ScopedNumericLocales&) = delete;
    ScopedNumericLocales& operator=(const ScopedNumericLocales&) = delete;

private:
    QLocale previousQtLocale;
    icu::Locale previousIcuLocale;
};
}  // namespace

class testInputField: public QObject
{
    Q_OBJECT

public:
    testInputField()
    {
        tests::initApplication();
        field = std::make_unique<Gui::InputField>();
    }

private Q_SLOTS:
    void test_MismatchedFormatterAndWidgetLocaleDoesNotMutateValue()  // NOLINT
    {
        ScopedNumericLocales locales("da_DK", "en_US");

        Gui::InputField input;
        input.setValue(Base::Quantity(10, "mm"));

        QCOMPARE(input.getQuantity(), Base::Quantity(10, "mm"));
        QCOMPARE(input.rawValue(), 10.0);
    }

    void test_ValidatorUsesOriginalTextBeforeLocaleFixup()  // NOLINT
    {
        ScopedNumericLocales locales("da_DK", "en_US");

        Gui::InputField input;
        input.setMaximum(500);

        QString text = QStringLiteral("10.00 mm");
        int pos = 0;

        QCOMPARE(input.validate(text, pos), QValidator::Acceptable);
    }

    void test_GroupedLocaleNumberIsNormalizedBeforeParse()  // NOLINT
    {
        ScopedNumericLocales locales("en_US", "en_US");

        Gui::InputField input;
        input.setText(QStringLiteral("1,010.00 mm"));

        QCOMPARE(input.getQuantity(), Base::Quantity(1010, "mm"));
        QCOMPARE(input.rawValue(), 1010.0);
    }

private:
    std::unique_ptr<Gui::InputField> field;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testInputField)

#include "InputField.moc"
