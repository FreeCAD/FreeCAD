// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ComponentZooCases.h"

#include <functional>
#include <limits>
#include <type_traits>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QGroupBox>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QToolBox>
#include <QToolButton>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <App/DocumentObject.h>
#include <App/ObjectIdentifier.h>
#include <Gui/ElideCheckBox.h>
#include <Gui/ElideLabel.h>
#include <Gui/ExpressionCompleter.h>
#include <Gui/FileDialog.h>
#include <Gui/FontScaledSVG.h>
#include <Gui/InputField.h>
#include <Gui/InputVector.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/SpinBox.h>
#include <Gui/StatusBarLabel.h>
#include <Gui/TextEdit.h>
#include <Gui/Widgets.h>

namespace
{
using Zoo::WidgetCase;

template<class T>
WidgetCase control(const char* variant = "", std::function<void(T&)> setup = {})
{
    return {variant, [setup] {
                auto* widget = new T(nullptr);
                if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
                    button->setText(QStringLiteral("Sample option"));
                }
                if (setup) {
                    setup(*widget);
                }
                return widget;
            }};
}

template<class T>
WidgetCase checkbox(Qt::CheckState state)
{
    const char* variant = state == Qt::Unchecked ? "unchecked"
        : state == Qt::Checked                   ? "checked"
                                                 : "mixed";
    return control<T>(variant, [state](auto& widget) {
        widget.setCheckState(state);
        if constexpr (std::is_same_v<T, Gui::ElideCheckBox>) {
            widget.setText(QStringLiteral("A long checkbox label that needs eliding"));
            widget.setFixedWidth(220);
        }
    });
}

void treeItems(QTreeWidget& tree)
{
    tree.setHeaderLabels({QStringLiteral("Name"), QStringLiteral("Value")});
    auto* parent = new QTreeWidgetItem(&tree, {QStringLiteral("Group"), QString()});
    auto* child = new QTreeWidgetItem(parent, {QStringLiteral("Selected"), QStringLiteral("12.5")});
    child->setCheckState(0, Qt::Checked);
    child->setSelected(true);
    auto* disabled = new QTreeWidgetItem(parent, {QStringLiteral("Disabled"), QStringLiteral("42")});
    disabled->setDisabled(true);
    tree.expandAll();
    tree.setFixedHeight(135);
}
}  // namespace

namespace Zoo
{
std::vector<WidgetCase> buttons()
{
    return {
        control<QPushButton>(),
        control<QPushButton>("default", [](auto& w) { w.setDefault(true); }),
        control<QPushButton>(
            "checked",
            [](auto& w) {
                w.setCheckable(true);
                w.setChecked(true);
            }
        ),
        control<QToolButton>("arrow", [](auto& w) { w.setArrowType(Qt::DownArrow); }),
        control<QToolButton>(
            "split menu",
            [](auto& w) {
                auto* menu = new QMenu(&w);
                menu->addAction(QStringLiteral("First action"));
                w.setMenu(menu);
                w.setPopupMode(QToolButton::MenuButtonPopup);
            }
        ),
        checkbox<QCheckBox>(Qt::Unchecked),
        checkbox<QCheckBox>(Qt::Checked),
        checkbox<QCheckBox>(Qt::PartiallyChecked),
        checkbox<Gui::ElideCheckBox>(Qt::Unchecked),
        checkbox<Gui::ElideCheckBox>(Qt::Checked),
        checkbox<Gui::ElideCheckBox>(Qt::PartiallyChecked),
        control<QRadioButton>("unchecked"),
        control<QRadioButton>(
            "checked",
            [](auto& w) {
                w.setAutoExclusive(false);
                w.setChecked(true);
            }
        ),
        control<Gui::ColorButton>(
            "opaque",
            [](auto& w) {
                w.setText(QString());
                w.setColor(QColor("#3288ed"));
            }
        ),
        control<Gui::ColorButton>(
            "transparent",
            [](auto& w) {
                w.setText(QString());
                w.setAllowTransparency(true);
                w.setColor(QColor(50, 136, 237, 100));
            }
        ),
        control<QDialogButtonBox>("OK / Cancel", [](auto& w) {
            w.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            for (auto* button : w.buttons()) {
                button->setIcon(QIcon());
            }
        }),
    };
}

std::vector<WidgetCase> text(App::DocumentObject* object)
{
    return {
        control<QLineEdit>("text", [](auto& w) { w.setText(QStringLiteral("Sample text")); }),
        control<QLineEdit>(
            "read-only",
            [](auto& w) {
                w.setText(QStringLiteral("Read-only text"));
                w.setReadOnly(true);
            }
        ),
        control<QLineEdit>(
            "placeholder",
            [](auto& w) { w.setPlaceholderText(QStringLiteral("Search")); }
        ),
        control<QLineEdit>(
            "password",
            [](auto& w) {
                w.setText(QStringLiteral("password"));
                w.setEchoMode(QLineEdit::Password);
            }
        ),
        control<Gui::ClearLineEdit>("", [](auto& w) { w.setText(QStringLiteral("Search text")); }),
        control<Gui::ModifierLineEdit>("", [](auto& w) { w.setText(QStringLiteral("Ctrl+Shift")); }),
        control<Gui::AccelLineEdit>(
            "",
            [](auto& w) { w.setKeySequence(QKeySequence(QStringLiteral("Ctrl+Shift+S"))); }
        ),
        control<Gui::ExpressionLineEdit>(
            "",
            [object](auto& w) {
                w.setDocumentObject(object);
                w.setText(QStringLiteral("Values.Length * 2"));
            }
        ),
        control<Gui::ExpLineEdit>(
            "bound",
            [object](auto& w) {
                w.bind(App::ObjectIdentifier(*object->getPropertyByName("BoundString")));
                w.setText(QStringLiteral("Sample text"));
            }
        ),
        control<QPlainTextEdit>(
            "",
            [](auto& w) {
                w.setPlainText(QStringLiteral("First line\nSecond line"));
                w.setFixedHeight(75);
            }
        ),
        control<QTextEdit>(
            "rich text",
            [](auto& w) {
                w.setHtml(QStringLiteral("<b>Bold</b>, <i>italic</i>, and plain text"));
                w.setFixedHeight(65);
            }
        ),
        control<Gui::ExpressionTextEdit>(
            "",
            [object](auto& w) {
                w.setDocumentObject(object);
                w.setPlainText(QStringLiteral("Values.Length * 2\n+ 5 mm"));
                w.setFixedHeight(75);
            }
        ),
        control<Gui::TextEdit>(
            "",
            [](auto& w) {
                w.setPlainText(QStringLiteral("result = 12.5\nprint(result)"));
                w.setFixedHeight(75);
            }
        ),
        control<Gui::PropertyListEditor>("", [](auto& w) {
            w.setPlainText(QStringLiteral("First item\nSecond item\nThird item"));
            w.setFixedHeight(85);
        }),
    };
}

std::vector<WidgetCase> numbers(App::DocumentObject* object)
{
    return {
        control<QSpinBox>(
            "",
            [](auto& w) {
                w.setRange(-100, 100);
                w.setValue(-42);
            }
        ),
        control<QDoubleSpinBox>("", [](auto& w) { w.setValue(12.5); }),
        control<Gui::IntSpinBox>("", [](auto& w) { w.setValue(42); }),
        control<Gui::IntSpinBox>(
            "bound",
            [object](auto& w) {
                w.bind(App::ObjectIdentifier(*object->getPropertyByName("BoundInteger")));
                w.setValue(42);
            }
        ),
        control<Gui::UIntSpinBox>(
            "large value",
            [](auto& w) {
                w.setRange(0, std::numeric_limits<unsigned>::max());
                w.setValue(4000000000U);
            }
        ),
        control<Gui::DoubleSpinBox>("", [](auto& w) { w.setValue(12.5); }),
        control<Gui::QuantitySpinBox>(
            "length",
            [](auto& w) {
                w.setUnit(Base::Unit::Length);
                w.setValue(12.5);
            }
        ),
        control<Gui::QuantitySpinBox>(
            "read-only",
            [](auto& w) {
                w.setUnit(Base::Unit::Length);
                w.setValue(12.5);
                w.setReadOnly(true);
            }
        ),
        control<Gui::QuantitySpinBox>(
            "bound",
            [object](auto& w) {
                w.setUnit(Base::Unit::Length);
                w.bind(App::ObjectIdentifier(*object->getPropertyByName("BoundLength")));
                w.setValue(12.5);
            }
        ),
        control<Gui::InputField>(
            "valid",
            [](auto& w) {
                w.setUnit(Base::Unit::Length);
                w.setValue(12.5);
            }
        ),
        control<Gui::InputField>(
            "invalid",
            [](auto& w) {
                w.setUnit(Base::Unit::Length);
                w.setProperty("invalidText", QStringLiteral("not a quantity"));
            }
        ),
        control<Gui::InputField>(
            "bound",
            [object](auto& w) {
                w.setUnit(Base::Unit::Length);
                w.bind(App::ObjectIdentifier(*object->getPropertyByName("BoundLength")));
                w.setValue(12.5);
            }
        ),
        control<QSlider>(
            "",
            [](auto& w) {
                w.setOrientation(Qt::Horizontal);
                w.setValue(40);
            }
        ),
        control<QProgressBar>("", [](auto& w) { w.setValue(40); }),
        control<Gui::LocationWidget>("", [](auto& w) {
            w.setPosition(Base::Vector3d(12.5, 25, -5));
            w.setFixedHeight(145);
        }),
    };
}

std::vector<WidgetCase> spinboxes(App::DocumentObject* object)
{
    std::vector<WidgetCase> cases;
    for (const auto symbols :
         {QAbstractSpinBox::UpDownArrows, QAbstractSpinBox::PlusMinus, QAbstractSpinBox::NoButtons}) {
        const char* variant = symbols == QAbstractSpinBox::UpDownArrows ? "arrows"
            : symbols == QAbstractSpinBox::PlusMinus                    ? "plus/minus"
                                                                        : "no buttons (OVP)";
        cases.push_back(control<Gui::QuantitySpinBox>(variant, [symbols](auto& w) {
            w.setButtonSymbols(symbols);
            w.setUnit(Base::Unit::Length);
            w.setValue(57.62);
            w.setFixedWidth(symbols == QAbstractSpinBox::NoButtons ? 100 : 140);
        }));
        cases.push_back(control<Gui::QuantitySpinBox>("bound", [object, symbols](auto& w) {
            w.setButtonSymbols(symbols);
            w.setUnit(Base::Unit::Length);
            w.bind(App::ObjectIdentifier(*object->getPropertyByName("BoundLength")));
            w.setValue(12.5);
            w.setFixedWidth(140);
        }));
    }
    return cases;
}

std::vector<WidgetCase> labels()
{
    return {
        control<QLabel>("plain", [](auto& w) { w.setText(QStringLiteral("Sample label")); }),
        control<QLabel>("rich text", [](auto& w) { w.setText(QStringLiteral("<b>Bold</b> label")); }),
        control<Gui::FontScaledSVG>(
            "",
            [](auto& w) { w.setSvg(QStringLiteral(":/icons/button_valid.svg")); }
        ),
        control<Gui::ElideLabel>(
            "",
            [](auto& w) {
                w.setText(QStringLiteral("A long label that needs eliding to fit the available space"));
                w.setFixedWidth(260);
            }
        ),
        control<Gui::UrlLabel>(
            "",
            [](auto& w) {
                w.setText(QStringLiteral("FreeCAD documentation"));
                w.setUrl(QStringLiteral("https://www.freecad.org"));
                w.setLaunchExternal(false);
            }
        ),
        control<Gui::StatefulLabel>(
            "error",
            [](auto& w) {
                w.registerState(QStringLiteral("error"), QColor("#e05555"));
                w.setState(QStringLiteral("error"));
                w.setText(QStringLiteral("Invalid value"));
            }
        ),
        control<Gui::StatefulLabel>(
            "warning",
            [](auto& w) {
                w.registerState(QStringLiteral("warning"), QColor("#b08020"));
                w.setState(QStringLiteral("warning"));
                w.setText(QStringLiteral("Value outside recommended range"));
            }
        ),
        control<Gui::StatusBarLabel>(
            "elided",
            [](auto& w) {
                w.setText(QStringLiteral("A long status message that needs eliding to fit"));
                w.setElideMode(Qt::ElideRight);
                w.setFixedWidth(260);
            }
        ),
        control<Gui::LabelButton>("", [](auto& w) { w.setValue(QStringLiteral("Sample value")); }),
        control<Gui::LabelEditor>("", [](auto& w) { w.setText(QStringLiteral("Sample label")); }),
        control<Gui::FileChooser>(
            "file",
            [](auto& w) { w.setFileName(QStringLiteral("/models/sample.FCStd")); }
        ),
        control<Gui::FileChooser>("directory", [](auto& w) {
            w.setMode(Gui::FileChooser::Directory);
            w.setFileName(QStringLiteral("/models"));
        }),
    };
}

std::vector<WidgetCase> views(App::DocumentObject* object)
{
    return {
        control<QListWidget>(
            "selection / checks",
            [](auto& w) {
                auto* first = new QListWidgetItem(QStringLiteral("Selected item"), &w);
                first->setSelected(true);
                auto* second = new QListWidgetItem(QStringLiteral("Checked item"), &w);
                second->setCheckState(Qt::Checked);
                auto* third = new QListWidgetItem(QStringLiteral("Disabled item"), &w);
                third->setFlags(third->flags() & ~Qt::ItemIsEnabled);
                w.setFixedHeight(100);
            }
        ),
        control<QTreeWidget>("expanded", [](auto& w) { treeItems(w); }),
        control<QTreeView>(
            "",
            [](auto& w) {
                auto* model = new QStandardItemModel(&w);
                model->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Value")});
                auto* parent = new QStandardItem(QStringLiteral("Group"));
                parent->appendRow(
                    {new QStandardItem(QStringLiteral("Child")),
                     new QStandardItem(QStringLiteral("12.5"))}
                );
                model->appendRow(parent);
                w.setModel(model);
                w.expandAll();
                w.setFixedHeight(115);
            }
        ),
        control<QTableWidget>(
            "headers / selection",
            [](auto& w) {
                w.setColumnCount(2);
                w.setRowCount(3);
                w.setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Value")});
                for (int row = 0; row < 3; ++row) {
                    w.setItem(row, 0, new QTableWidgetItem(QStringLiteral("Item %1").arg(row + 1)));
                    w.setItem(row, 1, new QTableWidgetItem(QString::number((row + 1) * 12.5)));
                }
                w.selectRow(1);
                w.setFixedHeight(145);
            }
        ),
        control<Gui::PropertyMapEditor>(
            "",
            [object](auto& w) {
                w.bind(App::ObjectIdentifier(*object->getPropertyByName("Map")));
                w.setMap(
                    {{QStringLiteral("Length"), QStringLiteral("12.5 mm")},
                     {QStringLiteral("Name"), QStringLiteral("Sample")}}
                );
                w.setFixedHeight(120);
            }
        ),
        control<Gui::ActionSelector>("", [](auto& w) {
            new QTreeWidgetItem(w.availableTreeWidget(), {QStringLiteral("Available action")});
            new QTreeWidgetItem(w.selectedTreeWidget(), {QStringLiteral("Selected action")});
            w.setFixedHeight(180);
        }),
    };
}

std::vector<WidgetCase> containers()
{
    return {
        control<QComboBox>(
            "list",
            [](auto& w) {
                w.addItems({QStringLiteral("First option"), QStringLiteral("Second option")});
            }
        ),
        control<QComboBox>(
            "editable",
            [](auto& w) {
                w.setEditable(true);
                w.addItems({QStringLiteral("Custom value"), QStringLiteral("Second option")});
            }
        ),
        control<QFontComboBox>("", [](auto& w) { w.setCurrentFont(QApplication::font()); }),
        control<QGroupBox>(
            "checked",
            [](auto& w) {
                w.setTitle(QStringLiteral("Options"));
                w.setCheckable(true);
                auto* layout = new QVBoxLayout(&w);
                layout->addWidget(new QLineEdit(QStringLiteral("Sample value")));
            }
        ),
        control<QGroupBox>(
            "unchecked",
            [](auto& w) {
                w.setTitle(QStringLiteral("Options"));
                w.setCheckable(true);
                auto* layout = new QVBoxLayout(&w);
                layout->addWidget(new QLineEdit(QStringLiteral("Sample value")));
                w.setChecked(false);
            }
        ),
        control<QTabWidget>(
            "",
            [](auto& w) {
                w.addTab(new QLabel(QStringLiteral("First page")), QStringLiteral("First"));
                w.addTab(new QLabel(QStringLiteral("Second page")), QStringLiteral("Second"));
                w.setTabEnabled(1, false);
                w.setFixedHeight(80);
            }
        ),
        control<QToolBox>(
            "",
            [](auto& w) {
                w.addItem(new QLabel(QStringLiteral("First page")), QStringLiteral("First"));
                w.addItem(new QLabel(QStringLiteral("Second page")), QStringLiteral("Second"));
                w.setFixedHeight(110);
            }
        ),
        control<QSplitter>(
            "",
            [](auto& w) {
                w.addWidget(new QLineEdit(QStringLiteral("Left pane")));
                w.addWidget(new QLineEdit(QStringLiteral("Right pane")));
            }
        ),
        control<QScrollArea>(
            "scrollbars",
            [](auto& w) {
                auto* content = new QLabel(QStringLiteral("Scrollable content"));
                content->setFixedSize(800, 160);
                content->setAlignment(Qt::AlignLeft | Qt::AlignTop);
                w.setWidget(content);
                w.setFixedHeight(80);
            }
        ),
        control<QMenu>(
            "actions",
            [](auto& w) {
                w.setWindowFlags(Qt::Widget);
                w.addAction(QStringLiteral("Normal action"));
                auto* check = w.addAction(QStringLiteral("Checked action"));
                check->setCheckable(true);
                check->setChecked(true);
                w.addSeparator();
                w.addAction(QStringLiteral("Disabled action"))->setEnabled(false);
            }
        ),
        control<QMenuBar>(
            "",
            [](auto& w) {
                w.setNativeMenuBar(false);
                w.addMenu(QStringLiteral("File"));
                w.addMenu(QStringLiteral("Edit"));
            }
        ),
        control<QToolBar>("", [](auto& w) {
            w.addAction(QStringLiteral("First action"));
            w.addSeparator();
            w.addAction(QStringLiteral("Disabled"))->setEnabled(false);
        }),
    };
}

}  // namespace Zoo
