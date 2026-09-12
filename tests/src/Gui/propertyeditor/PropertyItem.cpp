// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QApplication>
#include <QFocusEvent>
#include <QLineEdit>
#include <QtTest/QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyStandard.h>
#include "Gui/SpinBox.h"
#include "Gui/Widgets.h"
#include "Gui/Application.h"

#include "Gui/propertyeditor/PropertyItem.h"
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

class MockPropertyItem final: public Gui::PropertyEditor::PropertyItem
{
public:
    MockPropertyItem() = default;

    void setTestValue(const QVariant& v)
    {
        testValue = v;
    }
    QVariant lastSetValue() const
    {
        return storedSetValue;
    }

protected:
    QVariant value(const App::Property* /*prop*/) const override
    {
        return testValue;
    }
    void setValue(const QVariant& v) override
    {
        storedSetValue = v;
    }

private:
    QVariant testValue;
    QVariant storedSetValue;
};

class testPropertyItem: public QObject
{
    Q_OBJECT

public:
    testPropertyItem()
    {
        tests::initApplication();
        // Gui::Application::Instance must not be nullptr when expression binding
        // calls into the macro manager during document transactions.
        if (!Gui::Application::Instance) {
            new Gui::Application(false);
        }
        item.reset(new MockPropertyItem());
    }

private Q_SLOTS:
    void init()
    {}

    void cleanup()
    {}

    void test_camelCaseSplitsTwoWords()  // NOLINT
    {
        item->setPropertyName(QLatin1String("CamelCase"));
        QCOMPARE(item->propertyName(), QLatin1String("Camel Case"));
    }

    void test_camelCaseSplitsThreeWords()  // NOLINT
    {
        item->setPropertyName(QLatin1String("ThreeWordProperty"));
        QCOMPARE(item->propertyName(), QLatin1String("Three Word Property"));
    }

    void test_digitBeforeUppercaseNotSplit()  // NOLINT
    {
        item->setPropertyName(QLatin1String("View3D"));
        QCOMPARE(item->propertyName(), QLatin1String("View3D"));
    }

    void test_camelCaseSplitButNotDigit()  // NOLINT
    {
        item->setPropertyName(QLatin1String("MyView3D"));
        QCOMPARE(item->propertyName(), QLatin1String("My View3D"));
    }

    void test_consecutiveUppercaseNotSplit()  // NOLINT
    {
        item->setPropertyName(QLatin1String("MyABC"));
        QCOMPARE(item->propertyName(), QLatin1String("My ABC"));
    }

    void test_notCleverEnoughToSplitConsecutiveCaps()  // NOLINT
    {
        item->setPropertyName(QLatin1String("MyABCOfDoom"));
        QCOMPARE(item->propertyName(), QLatin1String("My ABCOf Doom"));
    }

    void test_underscoresArentTheSameAsSpaces()  // NOLINT
    {
        item->setPropertyName(QLatin1String("Box_Length"));
        QCOMPARE(item->propertyName(), QLatin1String("Box_Length"));
    }

    void test_programmaticStringTextChangeDuringExpressionDiscardRestoresExpression()  // NOLINT
    {
        const auto docName = App::GetApplication().getUniqueDocumentName("test");
        auto* doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        auto* object = doc->addObject("App::VarSet", "VarSet");
        auto* source = freecad_cast<App::PropertyString*>(
            object->addDynamicProperty("App::PropertyString", "Source", "Variables")
        );
        auto* target = freecad_cast<App::PropertyString*>(
            object->addDynamicProperty("App::PropertyString", "Target", "Variables")
        );

        source->setValue("Hello");
        target->setValue("Hello");
        App::ObjectIdentifier path(*target);

        object->setExpression(
            path,
            std::shared_ptr<App::Expression>(App::Expression::parse(object, "Source"))
        );

        object->ExpressionEngine.execute();

        // Use the established factory (constructor is protected by design).

        std::unique_ptr<Gui::PropertyEditor::PropertyStringItem> stringItem(
            static_cast<Gui::PropertyEditor::PropertyStringItem*>(
                Gui::PropertyEditor::PropertyStringItem::create()
            )
        );

        stringItem->setPropertyData({target});
        QWidget parent;
        auto* editor = qobject_cast<Gui::ExpLineEdit*>(
            stringItem->createEditor(&parent, []() {}, Gui::PropertyEditor::FrameOption::NoFrame)
        );
        QVERIFY(editor);

        // Disable auto-apply: in real use PropertyItemDelegate manages this;
        // bypassing it here avoids Gui::Command::doCommand / MacroManager access.

        editor->setAutoApply(false);
        stringItem->setEditorData(
            editor,
            stringItem->data(Gui::PropertyEditor::PropertyItem::ValueColumn, Qt::EditRole)
        );

        // Invoke stashExpression() directly (private slot) to simulate a double-click.
        // Using QMetaObject ensures reliable behavior in headless/offscreen test environments.

        const bool invoked = QMetaObject::invokeMethod(editor, "stashExpression");
        QVERIFY(invoked);
        editor->setText(QStringLiteral("Hello"));  // programmatic — must NOT count as user edit
        QFocusEvent focusOut(QEvent::FocusOut);
        QApplication::sendEvent(editor, &focusOut);

        QVERIFY(object->getExpression(path).expression != nullptr);
        QCOMPARE(
            QString::fromStdString(object->getExpression(path).expression->toString()),
            QStringLiteral("Source")
        );
        App::GetApplication().closeDocument(docName.c_str());
    }

    void test_userStringEditDuringExpressionDiscardRemovesExpression()  // NOLINT
    {
        const auto docName = App::GetApplication().getUniqueDocumentName("test");
        auto* doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        auto* object = doc->addObject("App::VarSet", "VarSet");

        auto* source = freecad_cast<App::PropertyString*>(
            object->addDynamicProperty("App::PropertyString", "Source", "Variables")
        );
        auto* target = freecad_cast<App::PropertyString*>(
            object->addDynamicProperty("App::PropertyString", "Target", "Variables")
        );

        source->setValue("Hello");
        target->setValue("Hello");
        App::ObjectIdentifier path(*target);
        object->setExpression(
            path,
            std::shared_ptr<App::Expression>(App::Expression::parse(object, "Source"))
        );
        object->ExpressionEngine.execute();

        // Use the established factory (constructor is protected by design).
        std::unique_ptr<Gui::PropertyEditor::PropertyStringItem> stringItem(
            static_cast<Gui::PropertyEditor::PropertyStringItem*>(
                Gui::PropertyEditor::PropertyStringItem::create()
            )
        );
        stringItem->setPropertyData({target});

        QWidget parent;
        auto* editor = qobject_cast<Gui::ExpLineEdit*>(
            stringItem->createEditor(&parent, []() {}, Gui::PropertyEditor::FrameOption::NoFrame)
        );
        QVERIFY(editor);
        // Disable auto-apply: in real use PropertyItemDelegate manages this;
        // bypassing it here avoids Gui::Command::doCommand / MacroManager access.
        editor->setAutoApply(false);
        stringItem->setEditorData(
            editor,
            stringItem->data(Gui::PropertyEditor::PropertyItem::ValueColumn, Qt::EditRole)
        );

        // Invoke stashExpression() directly (private slot) to simulate a double-click.
        const bool invoked = QMetaObject::invokeMethod(editor, "stashExpression");
        QVERIFY(invoked);
        QTest::keyClicks(editor, QStringLiteral("Goodbye"));  // real user keyboard input
        QFocusEvent focusOut(QEvent::FocusOut);
        QApplication::sendEvent(editor, &focusOut);
        QVERIFY(object->getExpression(path).expression == nullptr);
        App::GetApplication().closeDocument(docName.c_str());
    }

    void test_numericExpressionDiscardStillUsesValueChange()  // NOLINT
    {
        const auto docName = App::GetApplication().getUniqueDocumentName("test");
        auto* doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        auto* object = doc->addObject("App::VarSet", "VarSet");
        auto* source = freecad_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Source", "Variables")
        );
        auto* target = freecad_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Target", "Variables")
        );

        source->setValue(100);
        target->setValue(100);
        App::ObjectIdentifier path(*target);
        object->setExpression(
            path,
            std::shared_ptr<App::Expression>(App::Expression::parse(object, "Source"))
        );
        object->ExpressionEngine.execute();

        // Use the established factory (constructor is protected by design).
        std::unique_ptr<Gui::PropertyEditor::PropertyIntegerItem> integerItem(
            static_cast<Gui::PropertyEditor::PropertyIntegerItem*>(
                Gui::PropertyEditor::PropertyIntegerItem::create()
            )
        );
        integerItem->setPropertyData({target});
        QWidget parent;
        auto* editor = qobject_cast<Gui::IntSpinBox*>(
            integerItem->createEditor(&parent, []() {}, Gui::PropertyEditor::FrameOption::NoFrame)
        );
        QVERIFY(editor);
        // Disable auto-apply: in real use PropertyItemDelegate manages this;
        // bypassing it here avoids Gui::Command::doCommand / MacroManager access.
        editor->setAutoApply(false);
        integerItem->setEditorData(
            editor,
            integerItem->data(Gui::PropertyEditor::PropertyItem::ValueColumn, Qt::EditRole)
        );

        // Call stashExpression() directly (public method on ExpressionSpinBox) to simulate
        // a double-click without requiring a visible/shown widget.

        editor->stashExpression();
        QFocusEvent restoreFocusOut(QEvent::FocusOut);
        QApplication::sendEvent(editor, &restoreFocusOut);
        QVERIFY(object->getExpression(path).expression != nullptr);
        editor->stashExpression();
        editor->setValue(150);  // spinbox API value change \u2014 should count as user edit
        QFocusEvent discardFocusOut(QEvent::FocusOut);
        QApplication::sendEvent(editor, &discardFocusOut);
        QVERIFY(object->getExpression(path).expression == nullptr);
        App::GetApplication().closeDocument(docName.c_str());
    }

private:
    std::unique_ptr<Gui::PropertyEditor::PropertyItem> item;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testPropertyItem)

#include "PropertyItem.moc"
