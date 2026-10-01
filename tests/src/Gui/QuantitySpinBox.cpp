// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QCoreApplication>
#include <QDebug>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QTest>
#include <QSignalSpy>
#include <QWheelEvent>

#include <App/Application.h>
#include <Base/UnitsApi.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/ObjectIdentifier.h>
#include <App/Property.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/VarSet.h>

#include "Gui/Application.h"
#include "Gui/MainWindow.h"
#include "Gui/EditableDatumLabel.h"
#include "Gui/View3DInventorViewer.h"
#include "Gui/QuantitySpinBox.h"
#include "Gui/PrefWidgets.h"
#include "Gui/SpinBox.h"
#include <src/LocaleTestHelpers.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers)

/// Gives the tests access to the line edit, so that input can be fed in the same way the user
/// types it. This is what fills the validated string that isNormalized() inspects.
class QuantitySpinBoxWithLineEdit: public Gui::QuantitySpinBox
{
public:
    using QAbstractSpinBox::lineEdit;
};

namespace
{
/// Schema names as registered in Base::UnitsSchemasData
const QString standardSchema = QStringLiteral("Internal");
const QString usCustomarySchema = QStringLiteral("Imperial");
const QString imperialDecimalSchema = QStringLiteral("ImperialDecimal");
const QString buildingUsSchema = QStringLiteral("ImperialBuilding");
class ScopedExpressionOwner
{
public:
    ScopedExpressionOwner()
        : documentName {App::GetApplication().getUniqueDocumentName("quantity_spinbox")}
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

class KeyEventParent: public QWidget
{
public:
    int returns {};
    int escapes {};

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            ++returns;
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Escape) {
            ++escapes;
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }
};
}  // namespace

class testQuantitySpinBox: public QObject
{
    Q_OBJECT

public:
    testQuantitySpinBox()
    {
        tests::initApplication();
        ensureGuiApplication();
    }

private Q_SLOTS:

    void initTestCase()
    {
        Base::UnitsApi::setSchema("Internal");
    }

    void init()
    {
        docName = App::GetApplication().getUniqueDocumentName("testQuantitySpinBox");
        App::DocumentInitFlags flags;
        flags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", flags);

        target = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Target"));
        QVERIFY(target != nullptr);
        targetFloat = freecad_cast<App::PropertyFloat*>(
            target->addDynamicProperty("App::PropertyFloat", "TargetFloat", "Variables")
        );
        QVERIFY(targetFloat != nullptr);
        targetInt = freecad_cast<App::PropertyInteger*>(
            target->addDynamicProperty("App::PropertyInteger", "TargetInt", "Variables")
        );
        QVERIFY(targetInt != nullptr);

        qsb = std::make_unique<Gui::QuantitySpinBox>();
    }

    void cleanup()
    {
        qsb.reset();
        if (doc) {
            App::GetApplication().closeDocument(docName.c_str());
        }
        doc = nullptr;
        target = nullptr;
        targetFloat = nullptr;
        targetInt = nullptr;
        docName.clear();
        // some tests switch the unit schema, the next one has to start from the default again
        Base::UnitsApi::setSchema("Internal");
    }

    void test_SimpleBaseUnit()  // NOLINT
    {
        auto result = qsb->valueFromText("1mm");
        QCOMPARE(result, Base::Quantity(1, "mm"));
    }

    void test_BareValueUsesDisplayedUnit()  // NOLINT
    {
        Base::UnitsApi::setSchema("ImperialDecimal");
        auto spinBox = lengthSpinBox(QStringLiteral("5"));

        QVERIFY(spinBox->hasValidInput());
        QCOMPARE(spinBox->valueFromText(QStringLiteral("5")), Base::Quantity(127, "mm"));
        QCOMPARE(spinBox->valueFromText(QStringLiteral("5 mm")), Base::Quantity(5, "mm"));

        Base::UnitsApi::setSchema("Internal");
        QuantitySpinBoxWithLineEdit customSchemaSpinBox;
        customSchemaSpinBox.setUnit(Base::Unit::Length);
        customSchemaSpinBox.setSchema(3);
        QCOMPARE(customSchemaSpinBox.valueFromText(QStringLiteral("5")), Base::Quantity(127, "mm"));
    }

    void test_BareValueUsesCurrentMagnitudeDependentDisplayUnit()  // NOLINT
    {
        Base::UnitsApi::setSchema("Internal");
        QuantitySpinBoxWithLineEdit spinBox;
        spinBox.setUnit(Base::Unit::Length);
        Base::Quantity quantity(20000.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);

        QCOMPARE(spinBox.text(), QStringLiteral("20.0 m"));
        QCOMPARE(spinBox.valueFromText(QStringLiteral("3")), Base::Quantity(3000.0, "mm"));
    }

    void test_BareValueUsesDisplayedUnitWithLocalizedDecimalSeparator()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "de_DE",
             .formattingLocale = "de_DE",
             .icuLocale = "de_DE",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(imperialDecimalSchema.toStdString());
        QuantitySpinBoxWithLineEdit spinBox;
        spinBox.setLocale(QLocale(QStringLiteral("de_DE")));
        spinBox.setUnit(Base::Unit::Length);
        Base::Quantity quantity(127.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);

        QCOMPARE(spinBox.text(), QStringLiteral("5,0 in"));
        QCOMPARE(spinBox.valueFromText(QStringLiteral("5,5")), Base::Quantity(139.7, "mm"));
        QCOMPARE(spinBox.valueFromText(QStringLiteral("5,5 mm")), Base::Quantity(5.5, "mm"));
    }

    void test_UnitInNumerator()  // NOLINT
    {
        auto result = qsb->valueFromText("1mm/10");
        QCOMPARE(result, Base::Quantity(0.1, "mm"));
    }

    void test_UnitInDenominator()  // NOLINT
    {
        auto result = qsb->valueFromText("1/10mm");
        QCOMPARE(result, Base::Quantity(0.1, "mm"));
    }

    void test_KeepFormat()  // NOLINT
    {
        auto quant = qsb->value();
        auto format = quant.getFormat();
        format.setPrecision(7);
        quant.setFormat(format);

        qsb->setValue(quant);

        auto val1 = qsb->value();
        QCOMPARE(val1.getFormat().getPrecision(), 7);

        // format shouldn't change after setting a double
        qsb->setValue(3.5);
        auto val2 = qsb->value();
        QCOMPARE(val2.getFormat().getPrecision(), 7);
    }

    void test_isNormalized_data()  // NOLINT
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("normalized");

        // A bare number is already a solution, whatever its formatting
        QTest::newRow("integer") << "5" << true;
        QTest::newRow("zero") << "0" << true;
        QTest::newRow("decimals") << "5.00" << true;

        // Attaching a unit needs no calculation either, and neither does the whitespace
        // between the number and its unit
        QTest::newRow("number with unit") << "5.00mm" << true;
        QTest::newRow("number with spaced unit") << "5.00 mm" << true;

        // A leading sign is part of the value, not something to be calculated. Whitespace
        // around it is irrelevant even though the canonical form has none.
        QTest::newRow("negative number") << "-5.00" << true;
        QTest::newRow("positive number") << "+5.00" << true;
        QTest::newRow("negative number with unit") << "-5.00mm" << true;
        QTest::newRow("positive number with unit") << "+5.00mm" << true;
        QTest::newRow("negative number with spaced unit") << "-5.00 mm" << true;
        QTest::newRow("negative number with spaced sign") << "- 5.00 mm" << true;
        QTest::newRow("positive number with spaced sign") << "+ 5.00 mm" << true;

        // Everything that can still be simplified has to be reported as not normalized, so
        // that the result gets shown before it is accepted
        QTest::newRow("multiplication") << "5*2" << false;
        QTest::newRow("multiplication with unit") << "5.00mm * 2" << false;
        QTest::newRow("division") << "10/2" << false;
        QTest::newRow("division with unit") << "5.00mm/2" << false;
        QTest::newRow("unit in denominator") << "1/10mm" << false;
        QTest::newRow("addition") << "5+2" << false;
        QTest::newRow("addition with unit") << "5mm + 2mm" << false;
        QTest::newRow("subtraction") << "5-2" << false;
        QTest::newRow("subtraction with unit") << "5mm - 2mm" << false;
        QTest::newRow("power") << "2^3" << false;
        QTest::newRow("negated product") << "-5*2" << false;
        QTest::newRow("negated sum in parentheses") << "-(5+2)" << false;
    }

    void test_isNormalized()  // NOLINT
    {
        QFETCH(QString, input);
        QFETCH(bool, normalized);

        auto spinBox = lengthSpinBox(input);
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), normalized);
    }

    void test_isNormalizedImperial_data()  // NOLINT
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("normalized");

        // Feet, inches and fractions of an inch are how a value is written down in this
        // schema, so an input already in that form is a solution and not a pending
        // calculation, even though it is spelled with operators
        QTest::newRow("inches") << "6\"" << true;
        QTest::newRow("feet") << "2'" << true;
        QTest::newRow("named inches") << "5 in" << true;
        QTest::newRow("negative named inches") << "-5 in" << true;
        QTest::newRow("feet and inches") << "1' 6\"" << true;
        QTest::newRow("negative feet and inches") << "-1' 6\"" << true;
        QTest::newRow("fraction of an inch") << "1/2\"" << true;
        QTest::newRow("inches and fraction") << "1\" + 1/2\"" << true;

        // Sums and products that do not spell out a single value still have to be calculated
        // first, even when they are written with imperial units
        QTest::newRow("sum of inches") << "6\" + 1\"" << false;
        QTest::newRow("scaled inches") << "2\" * 3" << false;
        QTest::newRow("halved feet") << "1'/2" << false;
    }

    void test_isNormalizedImperial()  // NOLINT
    {
        QFETCH(QString, input);
        QFETCH(bool, normalized);

        Base::UnitsApi::setSchema(buildingUsSchema.toStdString());

        auto spinBox = lengthSpinBox(input);
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), normalized);
    }

    void test_isNormalizedImperialInMetricSchema()  // NOLINT
    {
        // The standard schema displays this as 457.20 mm, a value the user has not seen yet,
        // so it has to be shown before the input can be accepted
        Base::UnitsApi::setSchema(standardSchema.toStdString());

        auto spinBox = lengthSpinBox(QStringLiteral("1' 6\""));
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), false);
    }

    void test_isNormalizedDisplayedValue_data()  // NOLINT
    {
        QTest::addColumn<QString>("schema");
        QTest::addColumn<double>("millimetres");

        QTest::newRow("standard") << standardSchema << 38.1;
        QTest::newRow("us customary") << usCustomarySchema << 38.1;
        QTest::newRow("imperial decimal") << imperialDecimalSchema << 38.1;
        QTest::newRow("building us, whole inches") << buildingUsSchema << 25.4;
        QTest::newRow("building us, fraction") << buildingUsSchema << 12.7;
        QTest::newRow("building us, inches and fraction") << buildingUsSchema << 38.1;
        QTest::newRow("building us, feet and inches") << buildingUsSchema << 457.2;
    }

    void test_isNormalizedDisplayedValue()  // NOLINT
    {
        QFETCH(QString, schema);
        QFETCH(double, millimetres);

        Base::UnitsApi::setSchema(schema.toStdString());

        QuantitySpinBoxWithLineEdit spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(millimetres, Base::Unit::Length));

        // Whatever the spin box wrote out itself is the value the user is looking at, so
        // there is nothing left to show them and enter can be accepted right away
        QVERIFY(spinBox.hasValidInput());

        QCOMPARE(spinBox.isNormalized(), true);
    }

    void test_isNormalizedSignedConditional()  // NOLINT
    {
        // The operand of a sign is not necessarily a number or another operator, here it is a
        // conditional expression. It still has to be calculated before it can be accepted.
        auto spinBox = lengthSpinBox(QStringLiteral("-(1 > 0 ? 2 : 3)"));
        QVERIFY(spinBox->hasValidInput());

        QCOMPARE(spinBox->isNormalized(), false);
    }

    void test_MismatchedFormatterAndWidgetLocaleDoesNotMutateValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10, "mm"));

        QCOMPARE(spinBox.value(), Base::Quantity(10, "mm"));
        QCOMPARE(spinBox.rawValue(), 10.0);
    }

    void test_NativeDigitRoundTripUsesTheWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "fa_IR",
             .formattingLocale = "fa_IR",
             .icuLocale = "fa_IR",
             .useQtSeparators = true}
        };

        const QLocale persian(QStringLiteral("fa_IR"));
        Base::Quantity quantity(1234.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        Gui::QuantitySpinBox unbound;
        unbound.setLocale(persian);
        unbound.setKeyboardTracking(false);
        unbound.setValue(quantity);
        unbound.show();
        auto* unboundEdit = unbound.findChild<QLineEdit*>();
        QVERIFY(unboundEdit != nullptr);
        if (!unboundEdit) {
            return;
        }
        const QString formatted = unbound.text();
        QVERIFY(formatted.contains(QChar(0x06F1)));
        unboundEdit->setText(formatted);
        unbound.selectNumber();
        const int unitStart = formatted.indexOf(QStringLiteral(" mm"));
        QVERIFY(unitStart > 0);
        QCOMPARE(unboundEdit->selectedText(), formatted.left(unitStart));
        QTest::keyClick(&unbound, Qt::Key_Return);
        QCOMPARE(unbound.rawValue(), 1234.5);
        QVERIFY(unbound.hasValidInput());

        ScopedExpressionOwner owner;
        Gui::QuantitySpinBox bound;
        bound.setLocale(persian);
        bound.setKeyboardTracking(false);
        bound.bind(owner.getPath());
        bound.setUnit(Base::Unit::Length);
        bound.setValue(quantity);
        bound.show();
        auto* boundEdit = bound.findChild<QLineEdit*>();
        QVERIFY(boundEdit != nullptr);
        if (!boundEdit) {
            return;
        }
        boundEdit->setText(formatted);
        bound.selectNumber();
        QCOMPARE(boundEdit->selectedText(), formatted.left(unitStart));
        QTest::keyClick(&bound, Qt::Key_Return);
        QCOMPARE(bound.rawValue(), 1234.5);
        QVERIFY(bound.hasValidInput());
    }

    void test_LocaleChangeReformatsAndParsesWithTheNewWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setLocale(QLocale(QStringLiteral("en_US")));
        Base::Quantity quantity(1234.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        QVERIFY(spinBox.text().contains(QStringLiteral("1,234.5")));

        spinBox.setLocale(QLocale(QStringLiteral("de_DE")));
        QCoreApplication::processEvents();
        QVERIFY(spinBox.text().contains(QStringLiteral("1234,5")));

        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("2.345,6 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(spinBox.rawValue(), 2345.6);
        QVERIFY(spinBox.hasValidInput());
    }

    void test_DotGroupingDoesNotCorruptEditableInteger()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "de_DE",
             .formattingLocale = "de_DE",
             .icuLocale = "de_DE",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setLocale(QLocale(QStringLiteral("de_DE")));
        Base::Quantity quantity(1234.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 0);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);

        QCOMPARE(spinBox.rawValue(), 1234.0);
        QVERIFY(!spinBox.text().contains(QStringLiteral("1.234")));
        QCOMPARE(spinBox.text(), QStringLiteral("1234 mm"));

        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(spinBox.rawValue(), 1234.0);
        QVERIFY(spinBox.hasValidInput());
    }

    void test_SchemaFormattedTextDoesNotReparseExactValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema("Internal");
        Gui::QuantitySpinBox spinBox;
        spinBox.setLocale(QLocale(QStringLiteral("en_US")));

        Base::Quantity quantity(12345.67, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);
        QCOMPARE(spinBox.text(), QStringLiteral("12.35 m"));
        QCOMPARE(spinBox.rawValue(), 12345.67);

        spinBox.show();
        spinBox.setFocus();
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.rawValue(), 12345.67);
        QVERIFY(spinBox.hasValidInput());
    }

    void test_TextChangedIsEmittedForKeyboardTrackedEdits()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        QSignalSpy textChanged(&spinBox, &Gui::QuantitySpinBox::textChanged);

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }

        edit->setText(QStringLiteral("11 mm"));

        QCOMPARE(textChanged.count(), 1);
        QCOMPARE(textChanged.at(0).at(0).toString(), QStringLiteral("11 mm"));
    }

    void test_StepByCommitsTheCanonicalQuantity()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(imperialDecimalSchema.toStdString());
        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setSingleStep(1.0);
        spinBox.setValue(Base::Quantity(127.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        QSignalSpy changed(
            &spinBox,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged)
        );

        QTest::keyClick(&spinBox, Qt::Key_Up);
        QCOMPARE(spinBox.rawValue(), 152.4);
        QCOMPARE(changed.count(), 1);

        const QPoint center = spinBox.rect().center();
        QWheelEvent wheel(
            center,
            spinBox.mapToGlobal(center),
            QPoint(),
            QPoint(0, 120),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false
        );
        QCoreApplication::sendEvent(&spinBox, &wheel);

        QCOMPARE(spinBox.rawValue(), 177.8);
        QCOMPARE(changed.count(), 2);
    }

    void test_ProgrammaticSetValueEmitsChanged()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(standardSchema.toStdString());
        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(10.0);

        QSignalSpy changed(&spinBox, qOverload<double>(&Gui::QuantitySpinBox::valueChanged));

        spinBox.setValue(11.0);

        QCOMPARE(spinBox.rawValue(), 11.0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(0).toDouble(), 11.0);

        spinBox.setValue(11.0);
        QCOMPARE(changed.count(), 1);

        {
            const QSignalBlocker blocker(&spinBox);
            spinBox.setValue(12.0);
        }

        QCOMPARE(spinBox.rawValue(), 12.0);
        QCOMPARE(changed.count(), 1);
    }

    void test_SteppingSurvivesFocusLoss()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(standardSchema.toStdString());
        QWidget parent;
        Gui::QuantitySpinBox first(&parent);
        Gui::QuantitySpinBox second(&parent);
        first.setUnit(Base::Unit::Length);
        first.setSingleStep(1.0);
        first.setValue(Base::Quantity(10.0, "mm"));
        second.setValue(Base::Quantity(0.0, "mm"));
        parent.show();
        first.show();
        second.show();
        first.setFocus();

        QTest::keyClick(&first, Qt::Key_Up);
        QCOMPARE(first.rawValue(), 11.0);

        second.setFocus();
        QCoreApplication::processEvents();
        QCOMPARE(first.rawValue(), 11.0);
    }

    void test_WheelSteppingSurvivesFocusLoss()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(standardSchema.toStdString());
        QWidget parent;
        Gui::QuantitySpinBox first(&parent);
        Gui::QuantitySpinBox second(&parent);
        first.setUnit(Base::Unit::Length);
        first.setSingleStep(1.0);
        first.setValue(Base::Quantity(10.0, "mm"));
        second.setValue(Base::Quantity(0.0, "mm"));
        parent.show();
        first.show();
        second.show();
        first.setFocus();

        const QPoint center = first.rect().center();
        QWheelEvent wheel(
            center,
            first.mapToGlobal(center),
            QPoint(),
            QPoint(0, 120),
            Qt::NoButton,
            Qt::NoModifier,
            Qt::NoScrollPhase,
            false
        );
        QCoreApplication::sendEvent(&first, &wheel);
        QCOMPARE(first.rawValue(), 11.0);

        second.setFocus();
        QCoreApplication::processEvents();
        QCOMPARE(first.rawValue(), 11.0);
    }

    void test_SteppingPersistsThroughReturn()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(standardSchema.toStdString());
        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setSingleStep(1.0);
        spinBox.setValue(Base::Quantity(5.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        QTest::keyClick(&spinBox, Qt::Key_Up);
        const double steppedUp = spinBox.rawValue();
        QCOMPARE(steppedUp, 6.0);
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(spinBox.rawValue(), steppedUp);

        QTest::keyClick(&spinBox, Qt::Key_Down);
        const double steppedDown = spinBox.rawValue();
        QCOMPARE(steppedDown, 5.0);
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(spinBox.rawValue(), steppedDown);
    }

    void test_SteppingUsesAValidPendingEditorValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(imperialDecimalSchema.toStdString());
        QuantitySpinBoxWithLineEdit spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setSingleStep(1.0);
        spinBox.setKeyboardTracking(false);
        Base::Quantity quantity(127.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();

        // This is a valid candidate (8 in), but it has not been committed because tracking is
        // disabled. Stepping should treat it as the user's intended current value.
        spinBox.lineEdit()->setText(QStringLiteral("8"));
        QCOMPARE(spinBox.rawValue(), 127.0);

        spinBox.stepBy(1);

        QCOMPARE(spinBox.rawValue(), 228.6);
        QCOMPARE(spinBox.text(), QStringLiteral("9.0 in"));
        QVERIFY(spinBox.hasValidInput());
    }

    void test_SteppingReevaluatesThePendingDisplayUnit()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Base::UnitsApi::setSchema(standardSchema.toStdString());
        QuantitySpinBoxWithLineEdit spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setSingleStep(1.0);
        spinBox.setKeyboardTracking(false);
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 1);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();

        QCOMPARE(spinBox.text(), QStringLiteral("10.0 mm"));
        spinBox.lineEdit()->setText(QStringLiteral("20000"));
        QCOMPARE(spinBox.rawValue(), 10.0);

        // The pending value displays as 20 m, so one step means 21 m. Using the previous mm
        // display unit would incorrectly produce 20001 mm.
        spinBox.stepBy(1);

        QCOMPARE(spinBox.rawValue(), 21000.0);
        QCOMPARE(spinBox.text(), QStringLiteral("21.0 m"));
    }

    void test_UnboundQuantityGrammarPreservesComments()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);

        QCOMPARE(
            spinBox.valueFromText(QStringLiteral("1 mm [original input 12,34,567]")),
            Base::Quantity(1.0, "mm")
        );
    }

    void test_GroupedLocaleNumberIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("1.000,00 mm");

        QCOMPARE(result, Base::Quantity(1000, "mm"));
    }

    void test_CanonicalDecimalPointRemainsAcceptedInCommaLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "da_DK",
             .formattingLocale = "en_US",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("10.00 mm");

        QCOMPARE(result, Base::Quantity(10, "mm"));
    }

    void test_IndianGroupedLocaleNumberIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_IN",
             .formattingLocale = "en_IN",
             .icuLocale = "en_IN",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText("12,34,567 mm");

        QCOMPARE(result, Base::Quantity(1234567, "mm"));
    }

    void test_GroupedScientificNotationIsNormalizedBeforeParse_data()  // NOLINT
    {
        QTest::addColumn<QString>("input");
        QTest::newRow("lowercase exponent") << QStringLiteral("1,234e5 mm");
        QTest::newRow("uppercase exponent") << QStringLiteral("1,234E5 mm");
    }

    void test_GroupedScientificNotationIsNormalizedBeforeParse()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        QFETCH(QString, input);

        Gui::QuantitySpinBox spinBox;
        const auto result = spinBox.valueFromText(input);

        QCOMPARE(result, Base::Quantity(123400000, "mm"));
    }

    void test_GroupedEditDoesNotCorruptRawValue()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");

        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();
        spinBox.selectNumber();
        QTest::keyClicks(&spinBox, "1,010.00");
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.rawValue(), 1010.0);
        QCOMPARE(spinBox.text(), QStringLiteral("1,010.00 mm"));
    }

    void test_BoundPrefSpinBoxGroupedDecimalUsesWidgetLocale()  // NOLINT
    {
        // Start aligned so the widget is constructed and initially displayed
        // using US numeric formatting.
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        Base::UnitsApi::setSchema("FEM");

        ScopedExpressionOwner owner;

        // Sketcher's datum dialog uses PrefQuantitySpinBox rather than a plain
        // QuantitySpinBox.
        Gui::PrefQuantitySpinBox spinBox;

        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);
        spinBox.bind(owner.getPath());

        spinBox.show();
        spinBox.setFocus();
        spinBox.selectNumber();

        {
            // Reproduce the suspected real-world mismatch:
            //
            //   widget locale:       en_US, decimal ".", grouping ","
            //   shared parse state:  decimal ",", grouping "."
            //
            // The widget still displays and accepts US-style text, but the
            // current implementation parses through the stale shared state.
            tests::ScopedNumericLocaleContext staleFormatting {
                {"en_US", ",", ".", "+", "-", 3, 3, "0"}
            };

            QTest::keyClicks(&spinBox, "12,345.67");
            QTest::keyClick(&spinBox, Qt::Key_Return);
        }

        QCOMPARE(spinBox.hasValidInput(), true);
        QCOMPARE(spinBox.rawValue(), 12345.67);
        QCOMPARE(spinBox.text(), QStringLiteral("12,345.67 mm"));
    }

    void test_BoundGroupedDecimalEditUsesWidgetLocale()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        Base::UnitsApi::setSchema("FEM");
        ScopedExpressionOwner owner;

        Gui::QuantitySpinBox spinBox;
        spinBox.bind(owner.getPath());
        spinBox.setUnit(Base::Unit::Length);

        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();
        spinBox.selectNumber();

        QTest::keyClicks(&spinBox, "12,345.67");
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.rawValue(), 12345.67);
    }

    void test_EffectiveSeparatorsOverrideFormattingLocaleId()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "pt_PT", .formattingLocale = "en_US", .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(1.5, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);

        spinBox.setValue(quantity);

        QCOMPARE(spinBox.text(), QStringLiteral("1,50 mm"));
        QCOMPARE(spinBox.text().at(1), QLocale().decimalPoint());
    }

    void test_FunctionArgumentSeparatorSurvivesQuantityParsing()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        ScopedExpressionOwner owner;

        Gui::QuantitySpinBox spinBox;
        spinBox.bind(owner.getPath());
        spinBox.setUnit(Base::Unit::Length);
        // Use a non-unit function name here. The helper-level min(1,234) case above still covers
        // separator preservation for that spelling, but the real parser tokenizes "min" as minute.
        const auto result = spinBox.valueFromText("pow(1, 234)");

        QCOMPARE(result.getValue(), 1.0);
    }

    void test_InvalidCommittedGroupingRemainsVisibleAndEscapes()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();

        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("12,34,567 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.text(), QStringLiteral("12,34,567 mm"));
        QCOMPARE(spinBox.rawValue(), 10.0);
        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(rejected.count(), 1);
        const auto arguments = rejected.at(0);
        QVERIFY(arguments.at(0).toString().contains(QStringLiteral("Malformed grouping")));
        QVERIFY(arguments.at(1).toInt() >= 0);
        QVERIFY(arguments.at(2).toInt() > 0);

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }
        QVERIFY(edit->property("numericInputInvalid").toBool());

        QTest::keyClick(&spinBox, Qt::Key_Escape);
        QCOMPARE(spinBox.text(), QStringLiteral("10.00 mm"));
        QCOMPARE(spinBox.rawValue(), 10.0);
        QVERIFY(spinBox.hasValidInput());
        QVERIFY(!edit->property("numericInputInvalid").toBool());
    }

    void test_ReturnSignalsMatchCommitResult()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        QSignalSpy changed(
            &spinBox,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged)
        );
        QSignalSpy returnPressed(&spinBox, &Gui::QuantitySpinBox::returnPressed);
        QSignalSpy editingFinished(&spinBox, &QAbstractSpinBox::editingFinished);
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }

        edit->setText(QStringLiteral("11 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(returnPressed.count(), 1);
        QCOMPARE(editingFinished.count(), 1);
        QCOMPARE(rejected.count(), 0);
        QCOMPARE(changed.count(), 1);

        edit->setText(QStringLiteral("12,34,567 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(returnPressed.count(), 1);
        QCOMPARE(editingFinished.count(), 1);
        QCOMPARE(rejected.count(), 1);
        QCOMPARE(changed.count(), 1);
    }

    void test_ActionKeysPropagateOnlyAfterSuccessfulHandling()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        KeyEventParent parent;
        Gui::QuantitySpinBox spinBox(&parent);
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        parent.show();
        spinBox.show();
        spinBox.setFocus();

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        edit->setText(QStringLiteral("11 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(parent.returns, 1);

        edit->setText(QStringLiteral("12,34,567 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QCOMPARE(parent.returns, 1);

        QTest::keyClick(&spinBox, Qt::Key_Escape);
        QCOMPARE(parent.escapes, 1);
        QCOMPARE(spinBox.rawValue(), 11.0);
    }

    void test_ClearedInputHasAnExplicitTransientState()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);

        QSignalSpy changed(&spinBox, qOverload<double>(&Gui::QuantitySpinBox::valueChanged));
        QSignalSpy cleared(&spinBox, &Gui::QuantitySpinBox::inputCleared);

        edit->setText(QStringLiteral("12 mm"));
        QCOMPARE(spinBox.rawValue(), 12.0);
        QCOMPARE(changed.count(), 1);

        QTest::keyClick(edit, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(edit->selectedText(), edit->text());
        QTest::keyClick(edit, Qt::Key_Backspace);
        QCOMPARE(edit->text(), QString {});
        QCOMPARE(cleared.count(), 1);
        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(changed.count(), 1);
        QCOMPARE(spinBox.rawValue(), 12.0);

        edit->setText(QStringLiteral("13 mm"));
        QVERIFY(spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 13.0);
    }

    void test_MultibyteDiagnosticSelectsTheCompleteSeparator()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "fr_FR",
             .formattingLocale = "fr_FR",
             .icuLocale = "fr_FR",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setLocale(QLocale(QStringLiteral("fr_FR")));
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();
        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }

        const QString narrowSpace = QString::fromUtf8("\xE2\x80\xAF");
        edit->setText(QStringLiteral("12") + narrowSpace + QStringLiteral("34 mm"));
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(rejected.count(), 1);
        const auto args = rejected.at(0);
        const int start = args.at(1).toInt();
        const int length = args.at(2).toInt();
        QCOMPARE(edit->text().mid(start, length), narrowSpace);
        QCOMPARE(length, narrowSpace.size());
        QCOMPARE(length, 1);
        QVERIFY(!edit->text().contains(QChar::ReplacementCharacter));
    }

    void test_ValidEditClearsTheInvalidState()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();
        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }

        QTest::keyClicks(edit, QStringLiteral("12,34,567 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);
        QVERIFY(edit->property("numericInputInvalid").toBool());

        edit->setText(QStringLiteral("11 mm"));
        QVERIFY(!edit->property("numericInputInvalid").toBool());
        QVERIFY(edit->toolTip().isEmpty());
        QVERIFY(spinBox.hasValidInput());
    }

    void test_InvalidFocusLossCommitsOnce()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);
        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("12,34,567 mm"));
        QLineEdit other;
        other.show();
        other.setFocus();

        QTRY_COMPARE(rejected.count(), 1);
        QCOMPARE(spinBox.rawValue(), 10.0);
    }

    void test_ValidFocusLossCommitsOnce()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setKeyboardTracking(false);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();
        QSignalSpy changed(
            &spinBox,
            qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged)
        );
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);

        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("11 mm"));
        QLineEdit other;
        other.show();
        other.setFocus();
        QTRY_COMPARE(changed.count(), 1);
        QCOMPARE(rejected.count(), 0);

        QLineEdit another;
        another.show();
        another.setFocus();
        QTest::qWait(0);
        QCOMPARE(changed.count(), 1);
    }

    void test_IncompleteEditHasNoTooltipAndDoesNotCommit()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);

        edit->setText(QStringLiteral("-"));

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QVERIFY(edit->toolTip().isEmpty());
        QCOMPARE(rejected.count(), 0);

        QTest::keyClick(&spinBox, Qt::Key_Return);

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QCOMPARE(spinBox.text(), QStringLiteral("-"));
        QCOMPARE(rejected.count(), 1);
        QVERIFY(rejected.at(0).at(0).toString().contains(QStringLiteral("Incomplete number")));

        QTest::keyClick(&spinBox, Qt::Key_Escape);
        QCOMPARE(spinBox.text(), QStringLiteral("10.00 mm"));
        QVERIFY(spinBox.hasValidInput());
    }

    void test_IncompatibleUnitAndRangeDiagnosticsRemainVisible()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };

        Gui::QuantitySpinBox spinBox;
        spinBox.setUnit(Base::Unit::Length);
        spinBox.setValue(Base::Quantity(10.0, "mm"));
        spinBox.show();
        spinBox.setFocus();

        auto* edit = spinBox.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        if (!edit) {
            return;
        }
        QSignalSpy rejected(&spinBox, &Gui::QuantitySpinBox::inputRejected);

        edit->setText(QStringLiteral("1 s"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QCOMPARE(rejected.count(), 1);
        QVERIFY(rejected.at(0).at(0).toString().contains(QStringLiteral("Incompatible unit")));

        QTest::keyClick(&spinBox, Qt::Key_Escape);
        spinBox.setRange(0.0, 10.0);
        edit->setText(QStringLiteral("11 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QVERIFY(!spinBox.hasValidInput());
        QCOMPARE(spinBox.rawValue(), 10.0);
        QCOMPARE(rejected.count(), 2);
        QVERIFY(rejected.at(1).at(0).toString().contains(QStringLiteral("outside the allowed range")));
    }

    void test_ValidGroupedCommitUpdatesQuantity()  // NOLINT
    {
        tests::ScopedLocaleEnvironment localeState {
            {.qtLocale = "en_US",
             .formattingLocale = "en_US",
             .icuLocale = "en_US",
             .useQtSeparators = true}
        };
        Base::UnitsApi::setSchema("FEM");

        Gui::QuantitySpinBox spinBox;
        Base::Quantity quantity(10.0, "mm");
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, 2);
        format.option = Base::QuantityFormat::None;
        quantity.setFormat(format);
        spinBox.setValue(quantity);
        spinBox.show();
        spinBox.setFocus();

        spinBox.findChild<QLineEdit*>()->setText(QStringLiteral("12,345.67 mm"));
        QTest::keyClick(&spinBox, Qt::Key_Return);

        QCOMPARE(spinBox.text(), QStringLiteral("12,345.67 mm"));
        QCOMPARE(spinBox.rawValue(), 12345.67);
        QVERIFY(spinBox.hasValidInput());
    }

    void test_ExpressionCommitUpdatesPreview_data()  // NOLINT
    {
        QTest::addColumn<QString>("commitMethod");
        QTest::newRow("formula-dialog") << QStringLiteral("formula");
        QTest::newRow("inline-return") << QStringLiteral("return");
        QTest::newRow("inline-focus-out") << QStringLiteral("focus-out");
    }

    void test_ExpressionCommitUpdatesPreview()  // NOLINT
    {
        QFETCH(QString, commitMethod);
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());
        QSignalSpy changed(&spin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged));
        int previewUpdates = 0;
        // Match task panels: commit the value and recompute when the widget notifies them.
        QObject::connect(
            &spin,
            qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            &spin,
            [this, &previewUpdates](double value) {
                targetFloat->setValue(value);
                doc->recompute();
                ++previewUpdates;
            }
        );
        setEditorText(spin, QStringLiteral("10"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QVERIFY(previewUpdates > 0);
        QCOMPARE(targetFloat->getValue(), 10.0);
        changed.clear();
        previewUpdates = 0;

        if (commitMethod == QStringLiteral("formula")) {
            // Exercise the formula dialog's acceptance path independently of inline entry.
            std::shared_ptr<App::Expression> expression = App::ExpressionParser::parse(target, "42");
            static_cast<Gui::ExpressionSpinBox&>(spin).setExpression(expression);
        }
        else {
            setEditorText(spin, QStringLiteral("x=42"));
            if (commitMethod == QStringLiteral("return")) {
                QTest::keyClick(&spin, Qt::Key_Return);
            }
            else {
                QFocusEvent focusOut(QEvent::FocusOut, Qt::TabFocusReason);
                QCoreApplication::sendEvent(&spin, &focusOut);
            }
        }

        QCOMPARE(spin.rawValue(), 42.0);
        QVERIFY(target->getExpression(pathFloat()).expression != nullptr);
        // No explicit recompute here: accepting the expression must update the preview now.
        QCOMPARE(targetFloat->getValue(), 42.0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(previewUpdates, 1);
    }

    void test_OnViewExpressionSurvivesRepeatedCommit_data()
    {
        QTest::addColumn<int>("key");
        QTest::newRow("Return") << int(Qt::Key_Return);
        QTest::newRow("Tab") << int(Qt::Key_Tab);
        QTest::newRow("CtrlEnter") << int(Qt::Key_Enter);
        QTest::newRow("Click") << 0;
    }

    void test_OnViewExpressionSurvivesRepeatedCommit()
    {
        QFETCH(int, key);
        ensureGuiApplication();
        QWidget parent;
        Gui::View3DInventorViewer viewer(&parent);
        Gui::EditableDatumLabel parameter(&viewer, Base::Placement());
        connect(&parameter, &Gui::EditableDatumLabel::finishEditingOnAllOVPs, &parameter, [&parameter]() {
            parameter.commitPendingInlineExpression();
        });
        parameter.startEdit(0, nullptr, true);
        auto* spin = parent.findChild<Gui::QuantitySpinBox*>();
        QVERIFY(spin);
        auto* editor = spin->findChild<QLineEdit*>();
        editor->selectAll();
        QTest::keyClicks(editor, "ovp=0");
        if (key) {
            QTest::keyClick(
                editor,
                static_cast<Qt::Key>(key),
                key == Qt::Key_Enter ? Qt::ControlModifier : Qt::NoModifier
            );
        }
        else {
            QVERIFY(parameter.commitPendingInlineExpression());
        }
        const auto expression = parameter.constraintExpression();
        QVERIFY(!expression.empty());
        QVERIFY(parameter.commitPendingInlineExpression());
        QVERIFY(parameter.commitPendingInlineExpression());
        QCOMPARE(parameter.constraintExpression(), expression);
        editor->selectAll();
        QTest::keyClicks(editor, "12");
        QVERIFY(parameter.commitPendingInlineExpression());
        QVERIFY(parameter.constraintExpression().empty());
        editor->selectAll();
        QTest::keyClick(editor, Qt::Key_Backspace);
        QVERIFY(parameter.constraintExpression().empty());
    }

    void test_InlineAssignmentRangePolicy_data()
    {
        QTest::addColumn<bool>("enabled");
        QTest::addColumn<double>("value");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("lower") << true << 0.0 << true;
        QTest::newRow("upper") << true << 10.0 << true;
        QTest::newRow("below") << true << -1.0 << false;
        QTest::newRow("above") << true << 11.0 << false;
        QTest::newRow("disabled") << false << 11.0 << true;
    }

    void test_InlineAssignmentRangePolicy()
    {
        QFETCH(bool, enabled);
        QFETCH(double, value);
        QFETCH(bool, accepted);
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.setRange(0, 10);
        spin.checkRangeInExpression(enabled);
        spin.bind(pathFloat());
        setEditorText(spin, QStringLiteral("limit=%1").arg(value));
        QSignalSpy returned(&spin, &Gui::QuantitySpinBox::returnPressed);
        QTest::keyClick(&spin, Qt::Key_Return);
        QCOMPARE(returned.count(), accepted ? 1 : 0);
        QCOMPARE(doc->getObject("Parameters") != nullptr, accepted);
        QCOMPARE(target->getExpression(pathFloat()).expression != nullptr, accepted);
    }

    void test_InlineReferenceRangeAndInvalidBinding_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("rangeCheck");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("outside-reference") << QStringLiteral("Target.Other") << true << false;
        QTest::newRow("disabled-reference") << QStringLiteral("Target.Other") << false << true;
        QTest::newRow("invalid-reference") << QStringLiteral("Target.Missing") << false << false;
    }

    void test_InlineReferenceRangeAndInvalidBinding()
    {
        QFETCH(QString, input);
        QFETCH(bool, rangeCheck);
        QFETCH(bool, accepted);
        auto* other = static_cast<App::PropertyFloat*>(
            target->addDynamicProperty("App::PropertyFloat", "Other")
        );
        other->setValue(20);
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.setRange(0, 10);
        spin.checkRangeInExpression(rangeCheck);
        spin.bind(pathFloat());
        target->ExpressionEngine.setValue(pathFloat(), App::ExpressionParser::parse(target, "5"));
        QSignalSpy returned(&spin, &Gui::QuantitySpinBox::returnPressed);
        setEditorText(spin, input);
        QTest::keyClick(&spin, Qt::Key_Return);
        QCOMPARE(returned.count(), accepted ? 1 : 0);
        if (!accepted) {
            QCOMPARE(editorText(spin), input);
        }
        QCOMPARE(
            target->getExpression(pathFloat()).expression->toString(),
            accepted ? std::string("Other") : std::string("5")
        );
        QVERIFY(!doc->getObject("Parameters"));
    }

    void test_InlineAssignmentRollbackPreservesToolTransaction_data()
    {
        QTest::addColumn<bool>("toolTransaction");
        QTest::newRow("owned") << false;
        QTest::newRow("tool") << true;
    }

    void test_InlineAssignmentRollbackPreservesToolTransaction()
    {
        QFETCH(bool, toolTransaction);
        auto* parameters = doc->addObject("App::VarSet", "Parameters");
        auto* variable = static_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "cycle", "Variables")
        );
        variable->setValue(7);
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());
        target->ExpressionEngine.setValue(
            pathFloat(),
            App::ExpressionParser::parse(target, "Parameters.cycle")
        );
        const auto previous = target->getExpression(pathFloat()).expression->toString();
        if (toolTransaction) {
            doc->openTransaction("Pending tool edits");
            target->Label.setValue("Pending edit");
        }
        const auto transaction = doc->getBookedTransactionID();
        setEditorText(spin, QStringLiteral("cycle=Parameters.cycle+1"));
        QSignalSpy returned(&spin, &Gui::QuantitySpinBox::returnPressed);
        QTest::keyClick(&spin, Qt::Key_Return);
        QCOMPARE(returned.count(), 0);
        QCOMPARE(variable->getValue(), 7.0);
        QCOMPARE(target->getExpression(pathFloat()).expression->toString(), previous);
        QCOMPARE(doc->getBookedTransactionID(), transaction);
        if (toolTransaction) {
            QCOMPARE(QString::fromUtf8(target->Label.getValue()), QStringLiteral("Pending edit"));
            doc->abortTransaction();
        }
    }

    void test_InlineTargetCycleRollsBackAssignment_data()
    {
        QTest::addColumn<bool>("tool");
        QTest::addColumn<bool>("existing");
        QTest::newRow("owned-new") << false << false;
        QTest::newRow("owned-existing") << false << true;
        QTest::newRow("tool-new") << true << false;
        QTest::newRow("tool-existing") << true << true;
    }

    void test_InlineTargetCycleRollsBackAssignment()
    {
        QFETCH(bool, tool);
        QFETCH(bool, existing);
        App::VarSet* parameters = nullptr;
        if (existing) {
            parameters = static_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
            auto* variable = static_cast<App::PropertyFloat*>(
                parameters->addDynamicProperty("App::PropertyFloat", "cycle", "Variables")
            );
            variable->setValue(2);
            auto* dependent
                = parameters->addDynamicProperty("App::PropertyFloat", "dependent", "Variables");
            parameters->ExpressionEngine.setValue(
                App::ObjectIdentifier(*dependent),
                App::ExpressionParser::parse(parameters, "cycle*2")
            );
            parameters->ExpressionEngine.execute();
        }
        targetFloat->setValue(5);
        target->ExpressionEngine.setValue(pathFloat(), App::ExpressionParser::parse(target, "5"));
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());
        if (tool) {
            doc->openTransaction("Pending tool edits");
            target->Label.setValue("Pending edit");
        }
        const auto transaction = doc->getBookedTransactionID();
        const QString input = QStringLiteral("cycle=Target.TargetFloat");
        setEditorText(spin, input);
        QSignalSpy returned(&spin, &Gui::QuantitySpinBox::returnPressed);
        QTest::keyClick(&spin, Qt::Key_Return);
        QCOMPARE(returned.count(), 0);
        QCOMPARE(editorText(spin), input);
        QCOMPARE(doc->getBookedTransactionID(), transaction);
        QCOMPARE(targetFloat->getValue(), 5.0);
        QCOMPARE(target->getExpression(pathFloat()).expression->toString(), std::string("5"));
        if (existing) {
            QCOMPARE(
                static_cast<App::PropertyFloat*>(parameters->getPropertyByName("cycle"))->getValue(),
                2.0
            );
            QCOMPARE(
                static_cast<App::PropertyFloat*>(parameters->getPropertyByName("dependent"))->getValue(),
                4.0
            );
            QVERIFY(!parameters
                         ->getExpression(App::ObjectIdentifier(*parameters->getPropertyByName("cycle")))
                         .expression);
        }
        else {
            QVERIFY(!doc->getObject("Parameters"));
        }
        if (tool) {
            QCOMPARE(QString::fromUtf8(target->Label.getValue()), QStringLiteral("Pending edit"));
            doc->abortTransaction();
        }
    }

    void test_UIntInlineConversionLimits_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("negative") << QStringLiteral("limit=-1") << false;
        QTest::newRow("overflow") << QStringLiteral("limit=2147483647*2+2") << false;
        QTest::newRow("fractional-integer-assignment") << QStringLiteral("limit=1.4") << false;
        QTest::newRow("upper") << QStringLiteral("limit=2147483647*2+1") << true;
    }

    void test_UIntInlineConversionLimits()
    {
        QFETCH(QString, input);
        QFETCH(bool, accepted);
        Gui::UIntSpinBox spin;
        spin.setRange(0, std::numeric_limits<unsigned int>::max());
        QCOMPARE(spin.maximum(), std::numeric_limits<unsigned int>::max());
        setEditorText(spin, input);
        QCOMPARE(spin.maximum(), std::numeric_limits<unsigned int>::max());
        QTest::keyClick(&spin, Qt::Key_Return);
        QVERIFY2(
            (doc->getObject("Parameters") != nullptr) == accepted,
            qPrintable(spin.findChild<QLineEdit*>()->toolTip())
        );
        if (!accepted) {
            QCOMPARE(editorText(spin), input);
        }
    }

    void test_UIntInlineReferenceRoundsBeforeConversion()
    {
        auto* parameters = doc->addObject("App::VarSet", "Parameters");
        auto* fraction = static_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "fraction")
        );
        fraction->setValue(1.4);
        Gui::UIntSpinBox spin;
        spin.setRange(0, 1);
        setEditorText(spin, QStringLiteral("Parameters.fraction"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCOMPARE(spin.value(), 1U);
        fraction->setValue(1.5);
        const QString input = QStringLiteral("Parameters.fraction");
        setEditorText(spin, input);
        QTest::keyClick(&spin, Qt::Key_Return);
        QCOMPARE(spin.value(), 1U);
        QCOMPARE(editorText(spin), input);
    }

    void test_InlineAssignmentUndoRedo()
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());
        setEditorText(spin, QStringLiteral("undoValue=42"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QVERIFY(doc->getObject("Parameters"));
        QVERIFY(target->getExpression(pathFloat()).expression);
        doc->undo();
        QVERIFY(!doc->getObject("Parameters"));
        QVERIFY(!target->getExpression(pathFloat()).expression);
        doc->redo();
        QVERIFY(doc->getObject("Parameters"));
        QVERIFY(target->getExpression(pathFloat()).expression);
    }

    void test_InlineAssignmentCreatesParametersVarSet()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x=42"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
    }

    void test_UnqualifiedNameResolvesInQuantitySpinBox()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
        const QString exprText = QString::fromStdString(info.expression->toString());
        QVERIFY(exprText.contains(QStringLiteral("Parameters.x")));
    }

    void test_AssignmentRhsResolvesUnqualifiedName()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("y=x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* y = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("y"));
        QVERIFY(y != nullptr);
        auto yExpr = parameters->getExpression(App::ObjectIdentifier(*y));
        QVERIFY(yExpr.expression != nullptr);

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
    }

    void test_EnterCommitEmitsEditingFinished()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());
        QSignalSpy spy(&spin, &QAbstractSpinBox::editingFinished);

        setEditorText(spin, QStringLiteral("x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(spy.count(), 1);
    }

    void test_InlineAssignmentWorksInUIntSpinBox()  // NOLINT
    {
        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        setEditorText(spin, QStringLiteral("n=32"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* n = freecad_cast<App::PropertyInteger*>(parameters->getPropertyByName("n"));
        QVERIFY(n != nullptr);
        QCOMPARE(n->getValue(), 32L);

        auto info = target->getExpression(pathInt());
        QVERIFY(info.expression != nullptr);
    }

    void test_TypedEqualsAllowsInlineAssignmentInQuantitySpinBox()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        auto* edit = spin.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        edit->clear();
        edit->setFocus();
        QTest::keyClicks(edit, QStringLiteral("x=42"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 42.0);
    }

    void test_TypedEqualsAllowsInlineAssignmentInUIntSpinBox()  // NOLINT
    {
        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        auto* edit = spin.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);
        edit->clear();
        edit->setFocus();
        QTest::keyClicks(edit, QStringLiteral("n=32"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* n = freecad_cast<App::PropertyInteger*>(parameters->getPropertyByName("n"));
        QVERIFY(n != nullptr);
        QCOMPARE(n->getValue(), 32L);
    }

    void test_EmptyInputDoesNotEmitValueResetSignal()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);
        spin.setValue(10.0);

        auto* edit = spin.findChild<QLineEdit*>();
        QVERIFY(edit != nullptr);

        QSignalSpy spy(&spin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged));
        edit->clear();
        QCoreApplication::processEvents();

        QCOMPARE(spy.count(), 0);
    }

    void test_QuantityAssignmentReusesExistingIntegerVariableType()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(5);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x=42"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* sameX = parameters->getPropertyByName("x");
        QVERIFY(sameX == x);
        QCOMPARE(x->getValue(), 42L);
        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression != nullptr);
    }

    void test_QuantityAssignmentRejectsNonIntegerForExistingIntegerVariable()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(5);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x=42.5"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(x->getValue(), 5L);
        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression == nullptr);
    }

    void test_UIntEnterCommitAppliesExpression()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        setEditorText(spin, QStringLiteral("x+1"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathInt());
        QVERIFY(info.expression != nullptr);
        QVERIFY(
            QString::fromStdString(info.expression->toString()).contains(QStringLiteral("Parameters.x"))
        );
    }

    void test_UIntFocusOutCommitAppliesExpression()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyInteger*>(
            parameters->addDynamicProperty("App::PropertyInteger", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(41);

        Gui::UIntSpinBox spin;
        spin.bind(pathInt());

        setEditorText(spin, QStringLiteral("x+1"));
        QFocusEvent focusOut(QEvent::FocusOut, Qt::TabFocusReason);
        QCoreApplication::sendEvent(&spin, &focusOut);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathInt());
        QVERIFY(info.expression != nullptr);
        QVERIFY(
            QString::fromStdString(info.expression->toString()).contains(QStringLiteral("Parameters.x"))
        );
    }

    void test_InvalidExpressionIsRejectedOnEnter()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.bind(pathFloat());

        setEditorText(spin, QStringLiteral("x+"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto info = target->getExpression(pathFloat());
        QVERIFY(info.expression == nullptr);
        QCOMPARE(editorText(spin), QStringLiteral("x+"));
    }

    void test_OvaDistanceAssignmentCapturesExpressionAndCreatesLengthVar()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("len=25 mm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        const std::string exprText = spin.takeUnboundExpressionText();
        QVERIFY(!exprText.empty());
        QCOMPARE(spin.takeUnboundExpressionText(), std::string());
        QCOMPARE(QString::fromStdString(exprText), QStringLiteral("Parameters.len"));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* len = parameters->getPropertyByName("len");
        QVERIFY(len != nullptr);
        QVERIFY(len->getTypeId().isDerivedFrom(App::PropertyLength::getClassTypeId()));
    }

    void test_OvaAngleAssignmentCapturesExpressionAndCreatesAngleVar()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Angle);

        setEditorText(spin, QStringLiteral("ang=30 deg"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        const std::string exprText = spin.takeUnboundExpressionText();
        QVERIFY(!exprText.empty());
        QCOMPARE(spin.takeUnboundExpressionText(), std::string());
        QCOMPARE(QString::fromStdString(exprText), QStringLiteral("Parameters.ang"));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* ang = parameters->getPropertyByName("ang");
        QVERIFY(ang != nullptr);
        QVERIFY(ang->getTypeId().isDerivedFrom(App::PropertyAngle::getClassTypeId()));
    }

    void test_OvaZeroDistanceAssignmentStillCapturesExpression()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("w=0 mm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        const std::string exprText = spin.takeUnboundExpressionText();
        QVERIFY(!exprText.empty());
        QCOMPARE(QString::fromStdString(exprText), QStringLiteral("Parameters.w"));

        auto* parameters = freecad_cast<App::VarSet*>(doc->getObject("Parameters"));
        QVERIFY(parameters != nullptr);
        auto* w = parameters->getPropertyByName("w");
        QVERIFY(w != nullptr);
        QVERIFY(w->getTypeId().isDerivedFrom(App::PropertyLength::getClassTypeId()));
    }

    void test_OvaUnqualifiedReferenceStoresQualifiedExpression()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyLength*>(
            parameters->addDynamicProperty("App::PropertyLength", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(42.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("x"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(
            QString::fromStdString(spin.takeUnboundExpressionText()),
            QStringLiteral("Parameters.x")
        );
    }

    void test_OvaAssignmentFromVariableEvaluatesImmediately()  // NOLINT
    {
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* y = freecad_cast<App::PropertyLength*>(
            parameters->addDynamicProperty("App::PropertyLength", "y", "Variables")
        );
        QVERIFY(y != nullptr);
        y->setValue(37.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("x=y"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        auto* x = freecad_cast<App::PropertyLength*>(parameters->getPropertyByName("x"));
        QVERIFY(x != nullptr);
        QCOMPARE(x->getValue(), 37.0);
        QVERIFY(parameters->getExpression(App::ObjectIdentifier(*x)).expression != nullptr);
        QCOMPARE(
            QString::fromStdString(spin.takeUnboundExpressionText()),
            QStringLiteral("Parameters.x")
        );
    }

    void test_OvaLeadingEqualsAssignmentRejected()  // NOLINT
    {
        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);

        setEditorText(spin, QStringLiteral("=w=25 mm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QVERIFY(doc->getObject("Parameters") == nullptr);
        QCOMPARE(spin.takeUnboundExpressionText(), std::string());
    }

    void test_InlineExpressionCompatibleUnitIsAccepted()  // NOLINT
    {
        // A result whose dimension matches the field's unit is accepted. The input references a
        // parameter, which the quantity grammar rejects, so it is routed to the inline expression
        // path and exercises the inline implied-unit check.
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyLength*>(
            parameters->addDynamicProperty("App::PropertyLength", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(40.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);
        spin.setValue(Base::Quantity(10.0, "mm"));

        setEditorText(spin, QStringLiteral("x + 1 cm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(spin.value(), Base::Quantity(50.0, "mm"));
        QCOMPARE(spin.unit(), Base::Unit::Length);
        QVERIFY(spin.hasValidInput());
    }

    void test_InlineExpressionDimensionlessResultUsesFieldUnit()  // NOLINT
    {
        // A dimensionless result in a unitful field is interpreted in the field's unit.
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyFloat*>(
            parameters->addDynamicProperty("App::PropertyFloat", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(3.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);
        spin.setValue(Base::Quantity(10.0, "mm"));

        setEditorText(spin, QStringLiteral("x + 2"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(spin.value(), Base::Quantity(5.0, "mm"));
        QCOMPARE(spin.unit(), Base::Unit::Length);
        QVERIFY(spin.hasValidInput());
    }

    void test_InlineExpressionIncompatibleUnitIsRejected()  // NOLINT
    {
        // A result whose dimension differs from the field's unit is rejected; the value and unit
        // are left unchanged.
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyAngle*>(
            parameters->addDynamicProperty("App::PropertyAngle", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(30.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::Length);
        spin.setValue(Base::Quantity(10.0, "mm"));

        setEditorText(spin, QStringLiteral("x + 1 deg"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(spin.value(), Base::Quantity(10.0, "mm"));
        QCOMPARE(spin.unit(), Base::Unit::Length);
        QVERIFY(!spin.hasValidInput());
    }

    void test_InlineExpressionUnitfulResultInDimensionlessFieldDiscardsUnit()  // NOLINT
    {
        // A unitful result in a dimensionless field has its unit discarded (matching
        // DlgExpressionInput's "unit discarded" handling) instead of flipping the field's unit.
        auto* parameters = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "Parameters"));
        QVERIFY(parameters != nullptr);
        auto* x = freecad_cast<App::PropertyLength*>(
            parameters->addDynamicProperty("App::PropertyLength", "x", "Variables")
        );
        QVERIFY(x != nullptr);
        x->setValue(4.0);

        Gui::QuantitySpinBox spin;
        spin.setUnit(Base::Unit::One);
        spin.setValue(Base::Quantity(10.0, Base::Unit::One));

        setEditorText(spin, QStringLiteral("x + 1 mm"));
        QTest::keyClick(&spin, Qt::Key_Return);
        QCoreApplication::processEvents();

        QCOMPARE(spin.value(), Base::Quantity(5.0, Base::Unit::One));
        QCOMPARE(spin.unit(), Base::Unit::One);
        QVERIFY(spin.hasValidInput());
    }

private:
    /// Builds a length spin box holding the given input, entered the way the user types it.
    /// isNormalized() inspects the last input accepted by the validator, so callers have to
    /// check hasValidInput() as well, otherwise a rejected string silently leaves the
    /// previous one in place.
    static std::unique_ptr<QuantitySpinBoxWithLineEdit> lengthSpinBox(const QString& input)
    {
        auto spinBox = std::make_unique<QuantitySpinBoxWithLineEdit>();
        spinBox->setUnit(Base::Unit::Length);
        spinBox->lineEdit()->setText(input);

        return spinBox;
    }

    static void ensureGuiApplication()
    {
        if (!Gui::Application::Instance) {
            Gui::Application::initApplication();
            Gui::Application::initOpenInventor();
            guiApp = std::make_unique<Gui::Application>(true);
        }
        if (!Gui::getMainWindow()) {
            mainWindow = std::make_unique<Gui::MainWindow>();
        }
    }

    App::ObjectIdentifier pathFloat() const
    {
        return App::ObjectIdentifier(*targetFloat);
    }

    App::ObjectIdentifier pathInt() const
    {
        return App::ObjectIdentifier(*targetInt);
    }

    static void setEditorText(QAbstractSpinBox& spin, const QString& text)
    {
        auto* edit = spin.findChild<QLineEdit*>();
        Q_ASSERT(edit);
        edit->setText(text);
    }

    static QString editorText(QAbstractSpinBox& spin)
    {
        auto* edit = spin.findChild<QLineEdit*>();
        Q_ASSERT(edit);
        return edit->text();
    }

    static std::unique_ptr<Gui::Application> guiApp;
    static std::unique_ptr<Gui::MainWindow> mainWindow;
    std::unique_ptr<Gui::QuantitySpinBox> qsb;
    std::string docName;
    App::Document* doc {};
    App::VarSet* target {};
    App::PropertyFloat* targetFloat {};
    App::PropertyInteger* targetInt {};
};

std::unique_ptr<Gui::Application> testQuantitySpinBox::guiApp;
std::unique_ptr<Gui::MainWindow> testQuantitySpinBox::mainWindow;

// NOLINTEND(readability-magic-numbers)

QTEST_MAIN(testQuantitySpinBox)

#include "QuantitySpinBox.moc"
