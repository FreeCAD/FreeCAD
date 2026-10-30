// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QAbstractSpinBox>
#include <QApplication>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QTest>
#include <QTreeView>

#include <App/Application.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include <Gui/QuantitySpinBox.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/Materials.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>
#include <Mod/Material/Gui/MaterialDelegate.h>

// NOLINTBEGIN(readability-magic-numbers)

namespace
{
/// Builds the same row layout MaterialsEditor uses for physical properties:
/// name (property key in data()), value (material in data()), type and units.
QList<QStandardItem*> propertyRow(
    const std::shared_ptr<Materials::Material>& material,
    const QString& key
)
{
    auto model = Materials::ModelManager::getManager().getModel(
        Materials::ModelUUIDs::ModelUUID_Mechanical_Density
    );
    auto modelProperty = (*model)[key];

    auto propertyItem = new QStandardItem(modelProperty.getDisplayName());
    propertyItem->setData(key);

    auto valueItem = new QStandardItem(material->getPhysicalValueString(key));
    QVariant variant;
    variant.setValue(material);
    valueItem->setData(variant);

    auto typeItem = new QStandardItem(modelProperty.getPropertyType());
    auto unitsItem = new QStandardItem(modelProperty.getUnits());

    return {propertyItem, valueItem, typeItem, unitsItem};
}
}  // namespace

class testMaterialDelegate: public QObject
{
    Q_OBJECT

public:
    testMaterialDelegate()
    {
        tests::initApplication();
    }

private Q_SLOTS:

    void initTestCase()
    {
        Base::UnitsApi::setSchema("Internal");
    }

    // An unset quantity property is stored as NaN without units. Editing it for the first time
    // must store what the user typed, not silently fall back to 0.
    // see: https://github.com/FreeCAD/FreeCAD/issues/33201
    void test_FirstEditOfUnsetQuantityIsStored()  // NOLINT
    {
        auto material = densityMaterial();
        QVERIFY(material->getPhysicalProperty(densityKey)->isNull());

        PropertyView propertyView(material);
        auto editor = propertyView.openEditor();
        QVERIFY(editor != nullptr);
        auto lineEdit = editor->findChild<QLineEdit*>();
        QVERIFY(lineEdit != nullptr);

        // The editor must not present the internal NaN marker or lose the property units
        QVERIFY2(
            !lineEdit->text().contains(QStringLiteral("nan"), Qt::CaseInsensitive),
            qPrintable(lineEdit->text())
        );
        QCOMPARE(editor->unit(), Base::Quantity::parse("kg/m^3").getUnit());

        typeAndCommit(editor, QStringLiteral("7850"));

        auto property = material->getPhysicalProperty(densityKey);
        QVERIFY(!property->isNull());
        QCOMPARE(property->getValue().value<Base::Quantity>(), Base::Quantity(7850.0, "kg/m^3"));
    }

    void test_EditOfSetQuantityIsStored()  // NOLINT
    {
        auto material = densityMaterial();
        material->getPhysicalProperty(densityKey)->setQuantity(QStringLiteral("2700 kg/m^3"));

        PropertyView propertyView(material);
        auto editor = propertyView.openEditor();
        QVERIFY(editor != nullptr);

        // The editor starts from the stored value
        QCOMPARE(editor->value(), Base::Quantity(2700.0, "kg/m^3"));

        typeAndCommit(editor, QStringLiteral("7850"));

        QCOMPARE(
            material->getPhysicalProperty(densityKey)->getValue().value<Base::Quantity>(),
            Base::Quantity(7850.0, "kg/m^3")
        );
    }

private:
    inline static const QString densityKey = QStringLiteral("Density");

    static std::shared_ptr<Materials::Material> densityMaterial()
    {
        auto material = std::make_shared<Materials::Material>();
        material->addPhysical(Materials::ModelUUIDs::ModelUUID_Mechanical_Density);
        return material;
    }

    /// Replaces the number the way a user does, then commits with Enter
    static void typeAndCommit(Gui::QuantitySpinBox* editor, const QString& text)
    {
        editor->selectNumber();
        QTest::keyClicks(editor, text);
        QTest::keyClick(editor, Qt::Key_Return);
        // The item view commits the editor data through a queued call
        QCoreApplication::processEvents();
    }

    /// A property tree showing the material's density, edited through MaterialDelegate
    struct PropertyView
    {
        explicit PropertyView(const std::shared_ptr<Materials::Material>& material)
        {
            auto modelRoot = new QStandardItem(QStringLiteral("Density"));
            model.appendRow(modelRoot);
            modelRoot->appendRow(propertyRow(material, densityKey));
            valueIndex = model.index(0, 1, modelRoot->index());

            view.setModel(&model);
            view.setItemDelegateForColumn(1, &delegate);
            view.expandAll();
            view.show();
        }

        Gui::QuantitySpinBox* openEditor()
        {
            if (!QTest::qWaitForWindowExposed(&view)) {
                return nullptr;
            }
            view.setCurrentIndex(valueIndex);
            view.edit(valueIndex);
            return qobject_cast<Gui::QuantitySpinBox*>(view.indexWidget(valueIndex));
        }

        QStandardItemModel model;
        MatGui::MaterialDelegate delegate;
        QTreeView view;
        QModelIndex valueIndex;
    };
};

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testMaterialDelegate)

#include "MaterialDelegate.moc"
