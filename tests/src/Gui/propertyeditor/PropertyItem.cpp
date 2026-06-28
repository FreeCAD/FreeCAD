// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QDebug>
#include <QtTest/QTest>

#include <App/Application.h>

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

private:
    std::unique_ptr<Gui::PropertyEditor::PropertyItem> item;
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testPropertyItem)

#include "PropertyItem.moc"
