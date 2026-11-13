// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <memory>

#include <QAbstractAnimation>
#include <QApplication>
#include <QDialog>
#include <QEventLoop>
#include <QGridLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QScopeGuard>
#include <QScrollBar>
#include <QTimer>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Gui/InputField.h>
#include <Gui/MainWindow.h>
#include <Gui/propertyeditor/PropertyEditor.h>
#include <Gui/PythonWrapper.h>

#include "ComponentZoo.h"
#include "ComponentZooCases.h"

namespace
{
using namespace Gui::PropertyEditor;

const QStringList pages {
    "buttons",
    "text",
    "numbers",
    "themed-spinboxes",
    "labels",
    "views",
    "containers",
    "property-states",
    "property-scalars",
    "property-geometry",
    "property-collections",
    "property-editors-states",
    "property-editors-scalars",
    "property-editors-geometry",
    "property-editors-collections"
};

QWidget* controls(const std::vector<Zoo::WidgetCase>& cases)
{
    auto* sheet = new QDialog;
    auto* grid = new QGridLayout(sheet);
    grid->addWidget(new QLabel(QStringLiteral("Component")), 0, 0);
    grid->addWidget(new QLabel(QStringLiteral("Enabled")), 0, 1);
    grid->addWidget(new QLabel(QStringLiteral("Disabled")), 0, 2);
    grid->setColumnMinimumWidth(0, 290);
    grid->setColumnMinimumWidth(1, 340);
    grid->setColumnMinimumWidth(2, 340);

    int row = 1;
    for (const auto& sample : cases) {
        for (int column : {1, 2}) {
            auto* widget = sample.make();
            widget->setEnabled(column == 1);
            const int width = std::min(widget->maximumWidth(), 340);
            widget->setMaximumWidth(width);
            widget->setMinimumWidth(std::min(width, 260));
            grid->addWidget(widget, row, column);
            if (column != 1) {
                continue;
            }
            const QString type = QString::fromUtf8(widget->metaObject()->className());
            const QString variant = QString::fromUtf8(sample.variant);
            auto* label = new QLabel(variant.isEmpty() ? type : type + "\n" + variant);
            grid->addWidget(label, row, 0);
        }
        ++row;
    }
    return sheet;
}

PropertyEditor* propertyView(QWidget* parent, PropertyModel::PropertyList properties)
{
    auto* view = new PropertyEditor(parent);
    view->buildUp(std::move(properties));
    view->expandAll();
    view->header()->setSectionResizeMode(QHeaderView::Fixed);
    view->header()->resizeSection(0, 240);
    view->header()->setStretchLastSection(true);
    view->setFixedWidth(600);
    return view;
}

void fitTree(PropertyEditor* view)
{
    int height = 2 * view->frameWidth() + 4;
    const auto visit = [&](auto&& self, const QModelIndex& parent) -> void {
        for (int row = 0; row < view->model()->rowCount(parent); ++row) {
            const auto index = view->model()->index(row, 0, parent);
            height += view->sizeHintForIndex(index).height();
            if (view->isExpanded(index)) {
                self(self, index);
            }
        }
    };
    visit(visit, QModelIndex());
    view->setFixedHeight(height);
}

QWidget* propertySheet(
    App::DocumentObject* object,
    const QString& page,
    const std::vector<std::string>& names
)
{
    auto* sheet = new QDialog;
    auto* grid = new QGridLayout(sheet);
    const int columns = page.endsWith("-states") ? 1 : 2;
    for (int column = 0; column < columns; ++column) {
        PropertyModel::PropertyList properties;
        int row = 1;
        const std::string prefix = column == 0 ? "Normal" : "ReadOnly";
        for (const auto& name : names) {
            if (columns == 2 && !name.starts_with(prefix)) {
                continue;
            }
            auto* property = object->getPropertyByName(name.c_str());
            if (page.startsWith("property-editors-")) {
                // Each view owns one active editor, as in the property panel.
                auto* view = propertyView(sheet, {{name, {property}}});
                view->setProperty("activateEditor", column == 0);
                grid->addWidget(view, row++, column);
                continue;
            }
            properties.emplace_back(name, std::vector<App::Property*> {property});
        }
        if (columns == 2) {
            grid->addWidget(
                new QLabel(column == 0 ? QStringLiteral("Editable") : QStringLiteral("Read-only")),
                0,
                column
            );
        }
        if (!page.startsWith("property-editors-")) {
            grid->addWidget(propertyView(sheet, std::move(properties)), columns - 1, column);
        }
    }
    return sheet;
}

QImage renderSheet(QWidget* sheet)
{
    // Capture the final state, not a frame of a clear-button fade or another transition.
    for (auto* animation : sheet->findChildren<QAbstractAnimation*>()) {
        if (animation->state() == QAbstractAnimation::Running && animation->totalDuration() > 0) {
            animation->setCurrentTime(animation->totalDuration());
        }
    }
    QApplication::processEvents();

    QImage image(sheet->size(), QImage::Format_ARGB32);
    image.setDotsPerMeterX(3780);
    image.setDotsPerMeterY(3780);
    image.fill(Qt::transparent);
    sheet->render(&image);
    return image;
}

void settleTheme()
{
    // Gui::ParamHandlers::addHandler() debounces changes for 100 ms; process its theme reload.
    QEventLoop loop;
    QTimer::singleShot(150, &loop, &QEventLoop::quit);
    loop.exec();
}

std::unique_ptr<QWidget> makeSheet(
    const QString& page,
    App::DocumentObject* object,
    const std::vector<std::string>& names
)
{
    if (page.startsWith("property-")) {
        return std::unique_ptr<QWidget>(propertySheet(object, page, names));
    }

    const auto cases = page == "buttons" ? Zoo::buttons()
        : page == "text"                 ? Zoo::text(object)
        : page == "numbers"              ? Zoo::numbers(object)
        : page == "themed-spinboxes"     ? Zoo::spinboxes(object)
        : page == "labels"               ? Zoo::labels()
        : page == "views"                ? Zoo::views(object)
                                         : Zoo::containers();
    return std::unique_ptr<QWidget>(controls(cases));
}

void openEditor(PropertyEditor* view)
{
    const auto visit = [&](auto&& self, const QModelIndex& parent) -> bool {
        for (int row = 0; row < view->model()->rowCount(parent); ++row) {
            const auto value = view->model()->index(row, 1, parent);
            if (value.flags() & Qt::ItemIsEditable) {
                view->openEditor(value);
                return true;
            }
            if (self(self, view->model()->index(row, 0, parent))) {
                return true;
            }
        }
        return false;
    };
    visit(visit, QModelIndex());
}

QImage snapshot(QWidget* zoo)
{
    zoo->setLayoutDirection(Qt::LeftToRight);
    zoo->ensurePolished();
    for (auto* view : zoo->findChildren<PropertyEditor*>()) {
        fitTree(view);
    }
    zoo->resize(zoo->layout()->sizeHint());
    zoo->layout()->activate();

    // Scroll areas need show events to lay out; keep the entire lifecycle offscreen.
    zoo->setAttribute(Qt::WA_DontShowOnScreen);
    zoo->show();
    zoo->setFocus();
    QApplication::processEvents();
    for (auto* view : zoo->findChildren<PropertyEditor*>()) {
        if (view->property("activateEditor").toBool()) {
            openEditor(view);
            fitTree(view);
        }
    }
    renderSheet(zoo);
    for (auto* view : zoo->findChildren<PropertyEditor*>()) {
        view->setFixedHeight(view->height() + view->verticalScrollBar()->maximum());
    }
    zoo->resize(zoo->layout()->sizeHint());
    zoo->layout()->activate();
    QApplication::processEvents();
    // InputField::showEvent() restores formatted text, so apply invalid edits afterward.
    for (auto* field : zoo->findChildren<Gui::InputField*>()) {
        const QVariant invalid = field->property("invalidText");
        if (!invalid.isValid()) {
            continue;
        }
        field->setText(invalid.toString());
    }
    return renderSheet(zoo);
}

}  // namespace

Py::Object TestGui::componentZoo(const char* themeName, const Py::List& samples)
{
    const QString theme = QString::fromUtf8(themeName);
    if (theme != "light" && theme != "dark") {
        throw Py::ValueError("Unknown component zoo theme");
    }

    const auto pack = ParameterManager::Create();
    const std::string packName = theme == "dark" ? "FreeCAD Dark" : "FreeCAD Light";
    const std::string config = App::Application::getResourceDir() + "Gui/PreferencePacks/"
        + packName + "/" + packName + ".cfg";
    if (pack->LoadDocument(config.c_str()) != 1) {
        throw Py::RuntimeError("Cannot read component zoo theme preferences");
    }

    auto& preferences = App::GetApplication().GetUserParameter();
    const auto saved = ParameterManager::Create();
    saved->CreateDocument();
    preferences.insertTo(Base::Reference<ParameterGrp>(saved));

    const QPalette palette = QApplication::palette();
    const QString previousStyle = qApp->styleSheet();
    auto* mainWindow = Gui::getMainWindow();
    const auto previousFile = mainWindow->property("fc_currentStyleSheet");
    const auto previousTiling = mainWindow->property("fc_tiledBackground");
    auto* active = App::GetApplication().getActiveDocument();
    const auto restoreStyle = qScopeGuard([&] {
        // Qt callbacks must not consume a pending Python exception during cleanup.
        PyObject *error = nullptr, *value = nullptr, *traceback = nullptr;
        PyErr_Fetch(&error, &value, &traceback);
        App::GetApplication().setActiveDocument(active);
        preferences.revert(Base::Reference<ParameterGrp>(pack));
        saved->insertTo(Base::Reference<ParameterGrp>(&preferences));
        settleTheme();
        mainWindow->setProperty("fc_currentStyleSheet", previousFile);
        mainWindow->setProperty("fc_tiledBackground", previousTiling);
        QApplication::setPalette(palette);
        qApp->setStyleSheet(previousStyle);
        PyErr_Restore(error, value, traceback);
    });

    QApplication::setPalette(QApplication::style()->standardPalette());
    // Import shipped preferences; production handlers select and load the theme.
    pack->insertTo(Base::Reference<ParameterGrp>(&preferences));
    settleTheme();

    Py::Dict images;
    Gui::PythonWrapper wrap;
    for (const auto& sample : samples) {
        const char* page = nullptr;
        PyObject* object = nullptr;
        PyObject* names = nullptr;
        if (!PyArg_ParseTuple(
                sample.ptr(),
                "sO!O!",
                &page,
                &App::DocumentObjectPy::Type,
                &object,
                &PyList_Type,
                &names
            )) {
            throw Py::Exception();
        }
        if (!pages.contains(QString::fromUtf8(page))) {
            throw Py::ValueError("Unknown component zoo page");
        }
        std::vector<std::string> properties;
        for (const auto& name : Py::List(names)) {
            properties.push_back(Py::String(name).as_std_string());
        }
        auto* target = static_cast<App::DocumentObjectPy*>(object)->getDocumentObjectPtr();
        App::GetApplication().setActiveDocument(target->getDocument());
        const auto zoo = makeSheet(QString::fromUtf8(page), target, properties);
        images[Py::String(page)] = wrap.fromQImage(snapshot(zoo.get()));
    }
    return images;
}
