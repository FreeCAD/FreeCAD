// SPDX-License-Identifier: LGPL-2.1-or-later

#include <string>

#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <App/Property.h>
#include <Base/UnitsApi.h>

#include "Gui/InputField.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
class ScopedExpressionOwner
{
public:
    ScopedExpressionOwner()
        : documentName {App::GetApplication().getUniqueDocumentName("input_field")}
        , document {App::GetApplication().newDocument(documentName.c_str(), "testUser")}
        , object {document->addObject("App::VarSet", "VarSet")}
        , property {object->addDynamicProperty("App::PropertyFloat", "Value", "Test")}
    {}

    ~ScopedExpressionOwner()
    {
        App::GetApplication().closeDocument(documentName.c_str());
    }

    App::ObjectIdentifier getPath() const
    {
        return App::ObjectIdentifier(*property);
    }

private:
    std::string documentName;
    App::Document* document;
    App::DocumentObject* object;
    App::Property* property;
};

class TestInputField: public Gui::InputField
{
public:
    using Gui::InputField::InputField;

    std::string expressionString() const
    {
        return getExpressionString(false);
    }
};
}  // namespace

class testInputField: public QObject
{
    Q_OBJECT

public:
    testInputField()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void initTestCase()
    {
        Base::UnitsApi::setSchema("Internal");
    }

    void test_MismatchedFormatterAndWidgetLocaleEditPreservesEnteredValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        input.setValue(quantity);
        input.show();
        input.setFocus();
        input.selectNumber();
        QTest::keyClicks(&input, "1.010,00");
        QTest::keyClick(&input, Qt::Key_Return);

        QCOMPARE(input.rawValue(), 1010.0);
        QCOMPARE(input.text(), QStringLiteral("1.010,00 mm"));
    }

    void test_NativeDigitRoundTripUsesTheWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "fa_IR",
             .formattingLocale = "fa_IR",
             .icuLocale = "fa_IR",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        input.setLocale(QLocale(QStringLiteral("fa_IR")));
        input.setUnit(Base::Unit::Length);
        Base::Quantity quantity(1234.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        input.setValue(quantity);
        input.show();
        const QString formatted = input.text();
        QVERIFY(formatted.contains(QChar(0x06F1)));
        input.setText(formatted);
        input.selectNumber();
        const int unitStart = formatted.indexOf(QStringLiteral(" mm"));
        QVERIFY(unitStart > 0);
        QCOMPARE(input.selectedText(), formatted.left(unitStart));
        QTest::keyClick(&input, Qt::Key_Return);

        QCOMPARE(input.rawValue(), 1234.5);
        QVERIFY(input.hasValidInput());
    }

    void test_LocaleChangeReformatsAndParsesWithTheNewWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        input.setLocale(QLocale(QStringLiteral("en_US")));
        Base::Quantity quantity(1234.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        input.setValue(quantity);
        input.show();
        QVERIFY(input.text().contains(QStringLiteral("1,234.5")));

        input.setLocale(QLocale(QStringLiteral("de_DE")));
        QCoreApplication::processEvents();
        QVERIFY(input.text().contains(QStringLiteral("1.234,5")));

        input.setText(QStringLiteral("2.345,6 mm"));
        QTest::keyClick(&input, Qt::Key_Return);
        QCOMPARE(input.rawValue(), 2345.6);
        QVERIFY(input.hasValidInput());
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

    void test_FailedBoundEditDoesNotCommitCandidateExpression()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        ScopedExpressionOwner owner;

        TestInputField input;
        input.bind(owner.getPath());
        input.setUnit(Base::Unit::Length);
        input.setText(QStringLiteral("2 mm"));

        QVERIFY(input.hasValidInput());
        QCOMPARE(input.rawValue(), 2.0);
        const auto previousExpression = QString::fromStdString(input.expressionString());
        QVERIFY(!previousExpression.isEmpty());

        // This parses as an expression but does not evaluate to NumberExpression. It must not
        // replace the last valid expression or reset the stored quantity to zero.
        input.setText(QStringLiteral("str(1)"));

        QVERIFY(!input.hasValidInput());
        QCOMPARE(input.rawValue(), 2.0);
        QCOMPARE(QString::fromStdString(input.expressionString()), previousExpression);
    }

    void test_ReturnPressedOnlyFollowsValidCommit()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        input.setUnit(Base::Unit::Length);
        input.setValue(Base::Quantity(10.0, "mm"));
        input.show();
        input.setFocus();

        QSignalSpy returnPressed(&input, &QLineEdit::returnPressed);
        input.setText(QStringLiteral("11 mm"));
        QTest::keyClick(&input, Qt::Key_Return);
        QCOMPARE(returnPressed.count(), 1);

        input.setText(QStringLiteral("12,34,567 mm"));
        QSignalSpy parseError(&input, &Gui::InputField::parseError);
        QTest::keyClick(&input, Qt::Key_Return);
        QCOMPARE(returnPressed.count(), 1);
        QCOMPARE(parseError.count(), 1);
    }

    void test_CommitPreservesQuantityFormatOnReturnAndFocusLoss()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::InputField input;
        input.setUnit(Base::Unit::Length);
        Base::Quantity quantity(10.0, "mm");
        auto format = quantity.getFormat();
        format.setPrecision(5);
        quantity.setFormat(format);
        input.setValue(quantity);
        input.show();
        input.setFocus();

        input.setText(QStringLiteral("11 mm"));
        QTest::keyClick(&input, Qt::Key_Return);
        QCOMPARE(input.getPrecision(), 5);

        input.setText(QStringLiteral("12 mm"));
        QLineEdit other;
        other.show();
        other.setFocus();
        QTRY_COMPARE(input.getPrecision(), 5);
    }
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testInputField)

#include "InputField.moc"
