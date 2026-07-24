// SPDX-License-Identifier: LGPL-2.1-or-later

#include <string>

#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <App/Property.h>

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
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testInputField)

#include "InputField.moc"
