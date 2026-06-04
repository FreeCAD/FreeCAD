// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QTest>

#include <cmath>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>

#include "Gui/Dialogs/DlgExpressionInput.h"
#include <src/App/InitApplication.h>

class testDlgExpressionInput: public QObject
{
    Q_OBJECT

public:
    testDlgExpressionInput()
    {
        tests::initApplication();
    }

private:
    void checkImpliedLengthExpression(const char* source, const char* expectedQuantity)
    {
        Base::UnitsApi::setSchema("ImperialDecimal");

        docName = App::GetApplication().getUniqueDocumentName("test");
        App::Document* doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        App::DocumentObject* obj = doc->addObject("App::VarSet", "VarSet");
        QVERIFY(obj != nullptr);
        App::Property* prop = obj->addDynamicProperty("App::PropertyLength", "Len");
        QVERIFY(prop != nullptr);
        App::ObjectIdentifier path(*prop);

        auto expr = App::Expression::parse(obj, source);
        auto exprShared = std::shared_ptr<const App::Expression>(expr.release());

        Gui::Dialog::DlgExpressionInput dlg(path, exprShared, Base::Unit::Length);
        dlg.accept();

        auto accepted = dlg.getExpression();
        QVERIFY(accepted != nullptr);

        auto evaluated = accepted->eval();
        auto value = evaluated->getValueAsAny();
        QVERIFY(value.type() == typeid(Base::Quantity));

        const Base::Quantity quantity = App::any_cast<Base::Quantity>(value);
        QVERIFY(!quantity.isDimensionless());
        QCOMPARE(quantity.getUnit(), Base::Unit::Length);

        const Base::Quantity expected = Base::Quantity::parse(expectedQuantity);
        QVERIFY(std::abs(quantity.getValue() - expected.getValue()) < 1e-12);
    }

private Q_SLOTS:

    void init()
    {}

    void cleanup()
    {
        if (!docName.empty()) {
            App::GetApplication().closeDocument(docName.c_str());
            docName.clear();
        }
        Base::UnitsApi::setSchema("Internal");
    }

    void test_ImplicitUnitForDimensionlessExpression()
    {
        checkImpliedLengthExpression("1/10", "0.1 in");
    }

    void test_ImpliedUnitIgnoresLengthExpression()
    {
        Base::UnitsApi::setSchema("Internal");

        docName = App::GetApplication().getUniqueDocumentName("test");
        App::Document* doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        App::DocumentObject* obj = doc->addObject("App::VarSet", "VarSet");
        QVERIFY(obj != nullptr);

        App::Property* width = obj->addDynamicProperty("App::PropertyLength", "Width");
        QVERIFY(width != nullptr);
        App::ObjectIdentifier widthPath(*width);
        width->setPathValue(widthPath, Base::Quantity::parse("2 mm"));

        App::Property* length = obj->addDynamicProperty("App::PropertyLength", "Length");
        QVERIFY(length != nullptr);
        App::ObjectIdentifier lengthPath(*length);

        auto expr = App::Expression::parse(obj, "Width");
        auto exprShared = std::shared_ptr<const App::Expression>(expr.release());

        Gui::Dialog::DlgExpressionInput dlg(lengthPath, exprShared, Base::Unit::Length);
        dlg.accept();

        auto accepted = dlg.getExpression();
        QVERIFY(accepted != nullptr);

        auto evaluated = accepted->eval();
        auto value = evaluated->getValueAsAny();
        QVERIFY(value.type() == typeid(Base::Quantity));

        const Base::Quantity quantity = App::any_cast<Base::Quantity>(value);
        QCOMPARE(quantity.getUnit(), Base::Unit::Length);
        QVERIFY(std::abs(quantity.getValue() - Base::Quantity::parse("2 mm").getValue()) < 1e-12);
    }

private:
    std::string docName;
};

QTEST_MAIN(testDlgExpressionInput)

#include "DlgExpressionInput.moc"
