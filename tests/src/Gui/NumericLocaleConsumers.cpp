// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QLineEdit>
#include <QLocale>
#include <QTextEdit>
#include <QTest>

#include <memory>
#include <string>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/ObjectIdentifier.h>
#include <App/Property.h>

#include <Base/Quantity.h>

#include "Gui/Dialogs/DlgExpressionInput.h"
#include "Gui/Dialogs/DlgUnitsCalculatorImp.h"
#include "Gui/InputField.h"
#include <src/App/InitApplication.h>
#include <src/LocaleTestHelpers.h>

namespace
{
class ScopedExpressionOwner
{
public:
    ScopedExpressionOwner()
        : documentName {App::GetApplication().getUniqueDocumentName("numeric_locale_consumers")}
        , document {App::GetApplication().newDocument(documentName.c_str(), "testUser")}
        , object {document->addObject("App::GeoFeature", "Owner")}
        , property {object->addDynamicProperty("App::PropertyFloat", "Value", "Test")}
    {}

    ~ScopedExpressionOwner()
    {
        App::GetApplication().closeDocument(documentName.c_str());
    }

    App::DocumentObject* objectPtr() const
    {
        return object;
    }

    App::ObjectIdentifier path() const
    {
        return App::ObjectIdentifier(*property);
    }

private:
    std::string documentName;
    App::Document* document;
    App::DocumentObject* object;
    App::Property* property;
};
}  // namespace

class testNumericLocaleConsumers: public QObject
{
    Q_OBJECT

public:
    testNumericLocaleConsumers()
    {
        tests::initApplication();
    }

private Q_SLOTS:
    void test_ExpressionDialogUsesExpressionWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK", .formattingLocale = "de_DE", .icuLocale = "fr_FR"}
        };
        ScopedExpressionOwner owner;
        auto initial = App::ExpressionParser::parse(owner.objectPtr(), "10 mm");
        std::shared_ptr<const App::Expression> initialExpression(std::move(initial));
        Gui::Dialog::DlgExpressionInput dialog(owner.path(), initialExpression, Base::Unit::Length);

        auto* editor = dialog.findChild<QTextEdit*>(QStringLiteral("expression"));
        QVERIFY(editor != nullptr);
        if (!editor) {
            return;
        }
        editor->setLocale(QLocale(QStringLiteral("en_US")));
        editor->setPlainText(QStringLiteral("12,345.67 mm"));

        const auto expression = dialog.getExpression();
        QVERIFY(expression != nullptr);
        if (!expression) {
            return;
        }
        auto evaluated = expression->eval();
        auto* number = freecad_cast<App::NumberExpression*>(evaluated.get());
        QVERIFY(number != nullptr);
        if (!number) {
            return;
        }
        QCOMPARE(number->getQuantity().getValue(), 12345.67);
    }

    void test_UnitsCalculatorUsesItsInputWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK", .formattingLocale = "de_DE", .icuLocale = "fr_FR"}
        };
        Gui::Dialog::DlgUnitsCalculator dialog;

        auto* valueInput = dialog.findChild<Gui::InputField*>(QStringLiteral("ValueInput"));
        auto* unitInput = dialog.findChild<QLineEdit*>(QStringLiteral("UnitInput"));
        auto* valueOutput = dialog.findChild<QLineEdit*>(QStringLiteral("ValueOutput"));
        QVERIFY(valueInput != nullptr);
        QVERIFY(unitInput != nullptr);
        QVERIFY(valueOutput != nullptr);
        if (!valueInput || !unitInput || !valueOutput) {
            return;
        }

        valueInput->setLocale(QLocale(QStringLiteral("en_US")));
        unitInput->setLocale(QLocale(QStringLiteral("en_US")));
        valueInput->setText(QStringLiteral("12,345.67 mm"));
        unitInput->setText(QStringLiteral("in"));

        QCOMPARE(valueInput->rawValue(), 12345.67);
        QVERIFY(!valueOutput->text().contains(QStringLiteral("unknown unit")));
        QVERIFY(valueOutput->text().contains(QStringLiteral("in")));
    }
};

QTEST_MAIN(testNumericLocaleConsumers)

#include "NumericLocaleConsumers.moc"
