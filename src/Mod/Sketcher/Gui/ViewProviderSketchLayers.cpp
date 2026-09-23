// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <charconv>
#include <cmath>
#include <Mod/Sketcher/App/LayerDefaults.h>
#include <set>
#include <QColor>
#include <QMenu>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <TopAbs_ShapeEnum.hxx>
#include <QMessageBox>
#include <App/Document.h>
#include <Base/Exception.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "EditModeCoinManager.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

using namespace SketcherGui;

int ViewProviderSketch::getGeometryCoinLayerCount() const
{
    return static_cast<int>(getSketchObject()->getLayers().size())
        * std::max(1, VisualLayerList.getSize());
}

int ViewProviderSketch::getGeometryCoinLayer(const Sketcher::GeometryFacade* geometry) const
{
    const auto layers = getSketchObject()->getLayers();
    auto it = layers.find(geometry->getGeometryLayerId());
    const int index = it == layers.end() ? 0 : static_cast<int>(std::distance(layers.begin(), it));
    const int visualCount = std::max(1, VisualLayerList.getSize());
    const int visual = getSafeGeomLayerId(geometry);
    return index * visualCount + (visual >= 0 && visual < visualCount ? visual : 0);
}

std::vector<int> ViewProviderSketch::getLayerOrder() const
{
    const auto layers = getSketchObject()->getLayers();
    std::vector<int> order;
    std::set<int> added;
    for (auto id : LayerOrder.getValues()) {
        if (layers.contains(id) && added.insert(id).second) {
            order.push_back(id);
        }
    }
    for (const auto& [id, name] : layers) {
        if (added.insert(id).second) {
            order.push_back(id);
        }
    }
    return order;
}

bool ViewProviderSketch::isLayerVisible(int layerId) const
{
    if (!areLayersEnabled()) {
        return true;
    }
    const auto& hidden = HiddenLayers.getValues();
    return std::find(hidden.begin(), hidden.end(), layerId) == hidden.end();
}

bool ViewProviderSketch::isCoinLayerVisible(int coinLayer) const
{
    const auto layers = getSketchObject()->getLayers();
    const int visualCount = std::max(1, VisualLayerList.getSize());
    const int index = coinLayer / visualCount;
    if (coinLayer < 0 || index >= static_cast<int>(layers.size())) {
        return false;
    }
    const auto& visual = VisualLayerList.getValues();
    return isLayerVisible(std::next(layers.begin(), index)->first)
        && (visual.empty() || visual[coinLayer % visualCount].isVisible());
}

bool ViewProviderSketch::isGeometryVisible(int geoId) const
{
    if (geoId == Sketcher::GeoEnum::HAxis || geoId == Sketcher::GeoEnum::VAxis) {
        return true;
    }
    if (geoId == Sketcher::GeoEnum::GeoUndef
        || (geoId >= 0 && geoId >= getSketchObject()->Geometry.getSize())
        || (geoId < 0 && -static_cast<long long>(geoId) > getSketchObject()->ExternalGeo.getSize())) {
        return false;
    }
    return isCoinLayerVisible(getGeometryCoinLayer(getSketchObject()->getGeometryFacade(geoId).get()));
}

bool ViewProviderSketch::isConstraintVisible(const Sketcher::Constraint* constraint) const
{
    // Group/text constraints also define geometry and are not solver equations.
    if (constraint->Type != Sketcher::Group && constraint->Type != Sketcher::Text
        && !getSketchObject()->constraintUsesLayers(constraint)) {
        return false;
    }
    for (int i = 0; constraint->hasElement(i); ++i) {
        const int id = constraint->getGeoId(i);
        if (id != Sketcher::GeoEnum::GeoUndef && !isGeometryVisible(id)) {
            return false;
        }
    }
    return true;
}

void ViewProviderSketch::setLayerVisible(int layerId, bool visible)
{
    if (!getSketchObject()->hasLayer(layerId) || isLayerVisible(layerId) == visible) {
        return;
    }
    auto hidden = HiddenLayers.getValues();
    std::erase(hidden, layerId);
    if (!visible) {
        hidden.push_back(layerId);
    }
    HiddenLayers.setValues(std::move(hidden));
}

void ViewProviderSketch::refreshLayers()
{
    if (isInEditMode()) {
        const auto selection = Gui::Selection().getSelectionEx(
            getSketchObject()->getDocument()->getName()
        );
        for (const auto& object : selection) {
            if (object.getObject() != getSketchObject()) {
                continue;
            }
            for (const auto& name : object.getSubNames()) {
                int id;
                Sketcher::PointPos pos;
                bool hidden = getSketchObject()->geoIdFromShapeType(name.c_str(), id, pos)
                    && !isGeometryVisible(id);
                if (name.starts_with("Constraint")) {
                    const int index = Sketcher::PropertyConstraintList::getIndexFromConstraintName(
                        name
                    );
                    const auto& constraints = getSketchObject()->Constraints.getValues();
                    hidden = index >= 0 && index < static_cast<int>(constraints.size())
                        && !isConstraintVisible(constraints[index]);
                }
                if (hidden) {
                    // The edit path (a Body, for a sketch inside one) owns the selection.
                    rmvSelection(name);
                }
            }
        }
        Gui::Selection().rmvPreselect();
        editCoinManager->updateGeometryLayersConfiguration();
        draw(false, true);
        signalElementsChanged();
        signalConstraintsChanged();
    }
    signalLayersChanged();
}

void ViewProviderSketch::appendLayerMenu(QMenu* menu)
{
    // Outside edit mode selection names refer to the Shape, whose element numbering skips
    // construction and external geometry, so they cannot address sketch geometry here.
    if (!isInEditMode() || !areLayersEnabled()) {
        return;
    }
    auto* sketch = getSketchObject();
    std::set<int> ids;
    for (const auto& object : Gui::Selection().getSelectionEx(sketch->getDocument()->getName())) {
        if (object.getObject() != sketch) {
            continue;
        }
        for (const auto& name : object.getSubNames()) {
            int id;
            Sketcher::PointPos pos;
            if (sketch->geoIdFromShapeType(name.c_str(), id, pos)
                && (id >= 0 || id <= Sketcher::GeoEnum::RefExt) && id != Sketcher::GeoEnum::GeoUndef
                && sketch->getGeometry(id)) {
                ids.insert(id);
            }
        }
    }
    if (ids.empty()) {
        return;
    }
    menu->addSeparator();
    auto* submenu = menu->addMenu(tr("Layer"));
    std::string arguments;
    for (int id : ids) {
        if (!arguments.empty()) {
            arguments += ",";
        }
        arguments += std::to_string(id);
    }
    for (const auto& [layer, name] : sketch->getLayers()) {
        // A '&' in a layer name is literal text, not a mnemonic.
        auto* action = submenu->addAction(
            QString::fromStdString(name).replace(QLatin1Char('&'), QStringLiteral("&&"))
        );
        action->setCheckable(true);
        action->setChecked(std::all_of(ids.begin(), ids.end(), [sketch, layer](int id) {
            return sketch->getGeometryLayer(id) == layer;
        }));
        const auto geometryLocked = [sketch](int id) {
            return sketch->isLayerLocked(sketch->getGeometryLayer(id));
        };
        action->setEnabled(
            !sketch->isLayerLocked(layer) && std::none_of(ids.begin(), ids.end(), geometryLocked)
        );
        QObject::connect(action, &QAction::triggered, submenu, [this, sketch, arguments, layer]() {
            sketch->getDocument()->openTransaction(
                QT_TRANSLATE_NOOP("Command", "Move geometry to layer")
            );
            try {
                Gui::cmdAppObjectArgs(sketch, "setGeometryLayer([%s], %d)", arguments.c_str(), layer);
                sketch->getDocument()->commitTransaction();
                refreshLayers();
            }
            // Commands report Python errors as Base::PyException, not a std::exception.
            catch (const Base::Exception& e) {
                sketch->getDocument()->abortTransaction();
                QMessageBox::warning(
                    Gui::getMainWindow(),
                    tr("Sketch Layers"),
                    QString::fromUtf8(e.what())
                );
            }
            catch (const std::exception& e) {
                sketch->getDocument()->abortTransaction();
                QMessageBox::warning(
                    Gui::getMainWindow(),
                    tr("Sketch Layers"),
                    QString::fromUtf8(e.what())
                );
            }
        });
    }
}

Base::Color ViewProviderSketch::getLayerColor(int layerId, const Base::Color& fallback) const
{
    const auto& values = LayerColors.getValues();
    const auto it = values.find(std::to_string(layerId));
    if (it != values.end()) {
        const QColor color(QString::fromStdString(it->second));
        if (color.isValid()) {
            return Base::Color(color.redF(), color.greenF(), color.blueF());
        }
    }
    return fallback;
}

unsigned int ViewProviderSketch::getLayerPattern(int layerId, unsigned int fallback) const
{
    const auto& values = LayerPatterns.getValues();
    const auto it = values.find(std::to_string(layerId));
    if (it != values.end()) {
        bool ok;
        const auto pattern = QString::fromStdString(it->second).toUInt(&ok);
        if (ok && pattern <= 0xffff) {
            return pattern;
        }
    }
    return fallback;
}

float ViewProviderSketch::getLayerLineWidth(int layerId, float fallback) const
{
    const auto& values = LayerLineWidths.getValues();
    const auto it = values.find(std::to_string(layerId));
    if (it != values.end()) {
        bool ok;
        const float width = QString::fromStdString(it->second).toFloat(&ok);
        if (ok && std::isfinite(width) && width >= 1.0F && width <= 64.0F) {
            return width;
        }
    }
    return fallback;
}

void ViewProviderSketch::initializeNewLayerStyles()
{
    const auto previous = std::move(knownLayerIds);
    const auto layers = getSketchObject()->getLayers();
    knownLayerIds.clear();
    for (const auto& [id, name] : layers) {
        knownLayerIds.insert(id);
    }
    const auto* doc = getSketchObject()->getDocument();
    if (isRestoring() || getSketchObject()->isRestoring()
        || doc->testStatus(App::Document::Restoring) || doc->isPerformingTransaction()) {
        return;
    }
    const auto defaults = Sketcher::layerDefaults();
    const auto color = defaults->GetASCII("Color", "");
    const auto pattern = defaults->GetInt("Pattern", -1);
    const auto width = defaults->GetFloat("LineWidth", 0);
    for (int id : knownLayerIds) {
        if (previous.contains(id)) {
            continue;
        }
        const auto key = std::to_string(id);
        if (!color.empty() && QColor(QString::fromStdString(color)).isValid()) {
            LayerColors.setValue(key, color);
        }
        if (pattern >= 0 && pattern <= 0xffff) {
            LayerPatterns.setValue(key, std::to_string(pattern));
        }
        if (std::isfinite(width) && width >= 1 && width <= 64) {
            LayerLineWidths.setValue(key, std::to_string(width));
        }
        if (!defaults->GetBool("UseSolvedStateColors", true)) {
            auto disabled = LayerSolverColorsDisabled.getValues();
            if (std::find(disabled.begin(), disabled.end(), id) == disabled.end()) {
                disabled.push_back(id);
                LayerSolverColorsDisabled.setValues(std::move(disabled));
            }
        }
    }
}

bool ViewProviderSketch::layerUsesSolvedColors(int layerId) const
{
    const auto& disabled = LayerSolverColorsDisabled.getValues();
    return getSketchObject()->layerUsesConstraints(layerId)
        && std::find(disabled.begin(), disabled.end(), layerId) == disabled.end();
}

int ViewProviderSketch::getLayerFromCoinIndex(int coinLayer) const
{
    const auto layers = getSketchObject()->getLayers();
    const int index = coinLayer / std::max(1, VisualLayerList.getSize());
    return index >= 0 && index < static_cast<int>(layers.size())
        ? std::next(layers.begin(), index)->first
        : 0;
}

void ViewProviderSketch::updateVisual()
{
    ViewProvider2DObject::updateVisual();
    updateLayerStyles();
}

void ViewProviderSketch::updateLayerStyles()
{
    if (!getObject() || !lineset) {
        return;
    }
    const auto* sketch = getSketchObject();
    const auto& shape = sketch->Shape.getShape();
    const int count = shape.countSubShapes(TopAbs_EDGE);
    if (count == 0) {
        lineset->linePatterns.setNum(0);
        lineset->lineWidths.setNum(0);
        return;
    }
    const auto fallback = LineColor.getValue();
    std::vector<SbColor> colors(count, SbColor(fallback.r, fallback.g, fallback.b));
    std::vector<int32_t> patterns(count, pcLineStyle->linePattern.getValue());
    std::vector<float> widths(count, pcLineStyle->lineWidth.getValue());
    std::map<long, int> geometryIds;
    for (int index = 0; index < sketch->Geometry.getSize(); ++index) {
        geometryIds.emplace(Sketcher::GeometryFacade::getId(sketch->Geometry[index]), index);
    }
    for (int edge = 1; edge <= count; ++edge) {
        const auto name = shape.getMappedName(Data::IndexedName::fromConst("Edge", edge));
        const auto text = name.toString();
        long stableId = 0;
        if (text.empty() || text.front() != 'g'
            || std::from_chars(text.data() + 1, text.data() + text.size(), stableId).ec
                != std::errc()) {
            continue;
        }
        const auto found = geometryIds.find(stableId);
        if (found == geometryIds.end()) {
            continue;
        }
        const int id = found->second;
        const auto facade = sketch->getGeometryFacade(id);
        if (facade->getConstruction() || facade->isInternalAligned()) {
            continue;
        }
        const int layer = sketch->getGeometryLayer(id);
        const auto color = getLayerColor(layer, fallback);
        colors[edge - 1] = SbColor(color.r, color.g, color.b);
        patterns[edge - 1] = getLayerPattern(layer, patterns[edge - 1]);
        widths[edge - 1] = getLayerLineWidth(layer, widths[edge - 1]);
    }
    pcLineBind->value = SoMaterialBinding::PER_FACE;
    pcLineMaterial->diffuseColor.setValues(0, count, colors.data());
    pcLineMaterial->diffuseColor.setNum(count);
    lineset->linePatterns.setValues(0, count, patterns.data());
    lineset->linePatterns.setNum(count);
    lineset->lineWidths.setValues(0, count, widths.data());
    lineset->lineWidths.setNum(count);
}
