// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <QColor>
#include <QCoreApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <Base/Tools.h>
#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/LayerWidget.h>
#include <Gui/Selection/Selection.h>
#include <Mod/Sketcher/App/LayerDefaults.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "TaskSketcherLayers.h"
#include "ViewProviderSketch.h"

using namespace SketcherGui;

namespace
{
constexpr const char* UseConstraintsKey = "UseConstraints";
constexpr const char* UseSolvedColorsKey = "UseSolvedStateColors";

int toLayerId(const std::string& id)
{
    return std::stoi(id);
}

template<typename T>
std::string joinIds(const std::vector<T>& ids)
{
    std::string result;
    for (auto id : ids) {
        if (!result.empty()) {
            result += ",";
        }
        result += std::to_string(id);
    }
    return result;
}

std::string pythonString(const std::string& text)
{
    return Base::Tools::escapeQuotesFromString(Base::Tools::escapedUnicodeFromUtf8(text.c_str()));
}

class SketchLayerModel: public Gui::LayerModel
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::TaskSketcherLayers)

public:
    explicit SketchLayerModel(ViewProviderSketch* view)
        : view(view)
    {}

    std::vector<Gui::LayerInfo> layers() const override
    {
        std::vector<Gui::LayerInfo> result;
        auto* sketch = view->getSketchObject();
        const int active = sketch->ActiveLayer.getValue();
        const auto names = sketch->getLayers();
        const auto width = edgeWidth();
        for (int id : view->getLayerOrder()) {
            Gui::LayerInfo layer;
            layer.id = std::to_string(id);
            layer.name = names.at(id);
            layer.style.color = view->getLayerColor(id, view->LineColor.getValue());
            layer.style.pattern = view->getLayerPattern(id);
            layer.style.lineWidth = view->getLayerLineWidth(id, static_cast<float>(width));
            layer.visible = view->isLayerVisible(id);
            layer.locked = sketch->isLayerLocked(id);
            layer.active = id == active;
            layer.removable = id != 0;
            result.push_back(std::move(layer));
        }
        return result;
    }

    App::Document* document() const override
    {
        return view->getSketchObject()->getDocument();
    }

    void addLayer(const std::string& name) override
    {
        auto* sketch = view->getSketchObject();
        Gui::cmdAppObjectArgs(sketch, "addLayer(u'%s')", pythonString(name).c_str());
        const int id = sketch->NextLayerId.getValue() - 1;
        Gui::cmdAppObjectArgs(sketch, "setActiveLayer(%d)", id);
    }

    void renameLayer(const std::string& id, const std::string& name) override
    {
        Gui::cmdAppObjectArgs(
            view->getSketchObject(),
            "renameLayer(%d, u'%s')",
            toLayerId(id),
            pythonString(name).c_str()
        );
    }

    bool prepareRemoveLayer(const std::string& layerId, QWidget* parent) override
    {
        const int id = toLayerId(layerId);
        auto* sketch = view->getSketchObject();
        removal = {};
        auto collect = [&](int geoId) {
            if (sketch->getGeometryLayer(geoId) == id) {
                removal.members.push_back(geoId);
            }
        };
        for (int geoId = 0; geoId < sketch->Geometry.getSize(); ++geoId) {
            collect(geoId);
        }
        for (int index = 2; index < sketch->ExternalGeo.getSize(); ++index) {
            collect(-index - 1);
        }
        if (removal.members.empty()) {
            return true;
        }
        QDialog dialog(parent);
        dialog.setObjectName(QStringLiteral("removeSketchLayerDialog"));
        dialog.setWindowTitle(tr("Remove Layer"));
        auto* layout = new QVBoxLayout(&dialog);
        auto* label = new QLabel(
            tr("Layer “%1” contains geometry. What would you like to do with it?")
                .arg(QString::fromStdString(sketch->getLayers().at(id))),
            &dialog
        );
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        layout->addWidget(label);
        auto* move = new QRadioButton(tr("Move geometry to another layer"), &dialog);
        move->setChecked(true);
        layout->addWidget(move);
        auto* target = new QComboBox(&dialog);
        target->setObjectName(QStringLiteral("sketchLayerDestination"));
        target->setToolTip(tr("Sets the layer that receives the geometry"));
        for (const auto& [otherId, name] : sketch->getLayers()) {
            if (otherId != id && !sketch->isLayerLocked(otherId)) {
                target->addItem(QString::fromStdString(name), otherId);
            }
        }
        layout->addWidget(target);
        auto* remove = new QRadioButton(tr("Delete geometry"), &dialog);
        remove->setObjectName(QStringLiteral("deleteSketchLayerGeometry"));
        layout->addWidget(remove);
        if (target->count() == 0) {
            remove->setChecked(true);
            move->setEnabled(false);
        }
        QObject::connect(move, &QRadioButton::toggled, target, &QWidget::setEnabled);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        buttons->button(QDialogButtonBox::Ok)->setText(tr("Remove Layer"));
        QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);
        if (dialog.exec() != QDialog::Accepted) {
            return false;
        }
        removal.destination = target->currentData().toInt();
        removal.deleteContents = remove->isChecked();
        return true;
    }

    void removeLayer(const std::string& layerId) override
    {
        const int id = toLayerId(layerId);
        auto* sketch = view->getSketchObject();
        if (!removal.members.empty()) {
            const auto members = joinIds(removal.members);
            if (removal.deleteContents) {
                Gui::cmdAppObjectArgs(sketch, "delGeometries([%s])", members.c_str());
            }
            else {
                Gui::cmdAppObjectArgs(
                    sketch,
                    "setGeometryLayer([%s], %d)",
                    members.c_str(),
                    removal.destination
                );
            }
        }
        // Removing the active layer continues drawing on the chosen destination.
        if (!removal.deleteContents && sketch->ActiveLayer.getValue() == id) {
            Gui::cmdAppObjectArgs(sketch, "setActiveLayer(%d)", removal.destination);
        }
        Gui::cmdAppObjectArgs(sketch, "removeLayer(%d)", id);
        removal = {};
        view->refreshLayers();
    }

    void setActiveLayer(const std::string& id) override
    {
        Gui::cmdAppObjectArgs(view->getSketchObject(), "setActiveLayer(%d)", toLayerId(id));
    }

    void setLayerVisible(const std::string& id, bool visible) override
    {
        view->setLayerVisible(toLayerId(id), visible);
    }

    void setLayerLocked(const std::string& id, bool locked) override
    {
        setListed("LockedLayers", false, toLayerId(id), locked);
    }

    void setLayerColor(const std::string& id, const Base::Color& color) override
    {
        const auto name = QColor::fromRgbF(color.r, color.g, color.b).name().toStdString();
        setViewMapValue("LayerColors", toLayerId(id), name);
    }

    void setLayerPattern(const std::string& id, unsigned int pattern) override
    {
        setViewMapValue("LayerPatterns", toLayerId(id), std::to_string(pattern));
    }

    void setLayerLineWidth(const std::string& id, double width) override
    {
        setViewMapValue("LayerLineWidths", toLayerId(id), QString::number(width, 'f', 1).toStdString());
    }

    void reorderLayers(const std::vector<std::string>& ids) override
    {
        std::vector<int> order;
        std::transform(ids.begin(), ids.end(), std::back_inserter(order), toLayerId);
        Gui::cmdAppObjectArgs(
            view->getSketchObject(),
            "ViewObject.LayerOrder = [%s]",
            joinIds(order).c_str()
        );
    }

    std::vector<Gui::LayerSetting> extraSettings() const override
    {
        Gui::LayerSetting constraints;
        constraints.key = UseConstraintsKey;
        constraints.label = tr("Use constraints");
        constraints.defaultsLabel = tr("Default use constraints");
        constraints.toolTip = tr(
            "Includes the layer's geometry in constraint solving and allows new constraints on "
            "it. When disabled, existing constraints involving the layer are inactive and "
            "hidden; enabling it again restores their previous states."
        );
        constraints.defaultsToolTip = tr(
            "Sets whether new layers and the default layer of new sketches take part in "
            "constraint solving"
        );
        constraints.defaultValue = true;

        Gui::LayerSetting solvedColors;
        solvedColors.key = UseSolvedColorsKey;
        solvedColors.label = tr("Use solved state colors");
        solvedColors.defaultsLabel = tr("Default solver colors");
        solvedColors.toolTip = tr(
            "Replaces the layer color with the solver status colors while editing the sketch. "
            "Applies to normal geometry only and requires the layer to use constraints."
        );
        solvedColors.defaultsToolTip = tr("Sets whether new layers show solver status colors");
        solvedColors.defaultValue = true;
        solvedColors.dependsOn = UseConstraintsKey;
        return {constraints, solvedColors};
    }

    QVariant setting(const std::string& id, const std::string& key) const override
    {
        if (key == UseConstraintsKey) {
            return view->getSketchObject()->layerUsesConstraints(toLayerId(id));
        }
        if (key == UseSolvedColorsKey) {
            return view->layerUsesSolvedColors(toLayerId(id));
        }
        return {};
    }

    void setSetting(const std::string& id, const std::string& key, const QVariant& value) override
    {
        // Both are stored as the list of layers where the setting is off.
        if (key == UseConstraintsKey) {
            setListed("UnconstrainedLayers", false, toLayerId(id), !value.toBool());
        }
        else if (key == UseSolvedColorsKey) {
            setListed("LayerSolverColorsDisabled", true, toLayerId(id), !value.toBool());
        }
    }

    void populateContextMenu(QMenu* menu, const std::string& layerId) override
    {
        const int id = toLayerId(layerId);
        menu->addAction(tr("Select Layer Geometry"), [this, id]() {
            selectLayerContents(id, true, false);
        });
        menu->addAction(tr("Select Layer Constraints"), [this, id]() {
            selectLayerContents(id, false, true);
        });
        menu->addAction(tr("Select Layer Contents"), [this, id]() {
            selectLayerContents(id, true, true);
        });
    }

    ParameterGrp::handle defaultsGroup() const override
    {
        return Sketcher::layerDefaults();
    }

    Gui::LayerStyle fallbackStyle() const override
    {
        Gui::LayerStyle style;
        style.color = view->LineColor.getValue();
        style.lineWidth = edgeWidth();
        return style;
    }

private:
    static long edgeWidth()
    {
        return App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/View")
            ->GetInt("EdgeWidth", 2);
    }

    void setListed(const char* property, bool viewProperty, int id, bool listed)
    {
        const auto* container = viewProperty
            ? static_cast<const App::PropertyContainer*>(view)
            : static_cast<const App::PropertyContainer*>(view->getSketchObject());
        auto values = static_cast<const App::PropertyIntegerList*>(
                          container->getPropertyByName(property)
        )
                          ->getValues();
        std::erase(values, id);
        if (listed) {
            values.push_back(id);
        }
        Gui::cmdAppObjectArgs(
            view->getSketchObject(),
            "%s%s = [%s]",
            viewProperty ? "ViewObject." : "",
            property,
            joinIds(values).c_str()
        );
    }

    void setViewMapValue(const char* property, int id, const std::string& value)
    {
        auto* sketch = view->getSketchObject();
        Gui::cmdAppObjectArgs(
            sketch,
            "ViewObject.%s = dict(%s.ViewObject.%s, **{'%d': '%s'})",
            property,
            Gui::Command::getObjectCmd(sketch).c_str(),
            property,
            id,
            value.c_str()
        );
    }

    void selectLayerContents(int layerId, bool geometry, bool constraints)
    {
        auto* sketch = view->getSketchObject();
        Gui::Selection().clearSelection();
        if (geometry) {
            auto select = [&](int geoId) {
                if (sketch->getGeometryLayer(geoId) != layerId) {
                    return;
                }
                if (sketch->getGeometry(geoId)->is<Part::GeomPoint>()) {
                    const int vertex = sketch->getVertexIndexGeoPos(geoId, Sketcher::PointPos::start);
                    if (vertex >= 0) {
                        view->addSelection("Vertex" + std::to_string(vertex + 1));
                    }
                }
                else if (geoId >= 0) {
                    view->addSelection("Edge" + std::to_string(geoId + 1));
                }
                else {
                    view->addSelection(
                        "ExternalEdge" + std::to_string(Sketcher::GeoEnum::RefExt - geoId + 1)
                    );
                }
            };
            for (int geoId = 0; geoId < sketch->Geometry.getSize(); ++geoId) {
                select(geoId);
            }
            for (int index = 2; index < sketch->ExternalGeo.getSize(); ++index) {
                select(-index - 1);
            }
        }
        if (constraints) {
            const auto& values = sketch->Constraints.getValues();
            for (size_t index = 0; index < values.size(); ++index) {
                const auto* constraint = values[index];
                for (int element = 0; constraint->hasElement(element); ++element) {
                    const int geoId = constraint->getGeoId(element);
                    if (geoId != Sketcher::GeoEnum::GeoUndef
                        && (geoId >= 0 || geoId <= Sketcher::GeoEnum::RefExt)
                        && sketch->getGeometryLayer(geoId) == layerId) {
                        view->addSelection("Constraint" + std::to_string(index + 1));
                        break;
                    }
                }
            }
        }
    }

    ViewProviderSketch* view;
    /// What prepareRemoveLayer() decided to do with the contents of the layer.
    struct
    {
        std::vector<int> members;
        int destination = 0;
        bool deleteContents = false;
    } removal;
};
}  // namespace

TaskSketcherLayers::TaskSketcherLayers(ViewProviderSketch* sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_Layers"), tr("Layers"), true, nullptr)
    , view(sketchView)
    , widget(new Gui::LayerWidget(std::make_unique<SketchLayerModel>(sketchView), this))
{
    setObjectName(QStringLiteral("sketchLayersTaskBox"));
    widget->tree()->setObjectName(QStringLiteral("sketchLayers"));
    groupLayout()->addWidget(widget);
    connection = view->signalLayersChanged.connect([this]() {
        // Property changes can occur inside the list's own notifications; update after them.
        QTimer::singleShot(0, this, &TaskSketcherLayers::updateVisibility);
        widget->model()->signalChanged();
    });
    updateVisibility();
}

TaskSketcherLayers::~TaskSketcherLayers() = default;

void TaskSketcherLayers::updateVisibility()
{
    // Do not show an unparented taskbox as a separate window during construction.
    if (parentWidget() || !view->areLayersEnabled()) {
        setVisible(view->areLayersEnabled());
    }
    if (!view->areLayersEnabled()) {
        widget->cancelAddingLayer();
    }
}
