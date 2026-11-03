// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QPointer>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <fastsignals/signal.h>

#include <Base/Color.h>
#include <Base/Parameter.h>
#include <FCGlobal.h>

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QMenu;
class QTreeWidget;
class QTreeWidgetItem;

namespace App
{
class Document;
}

namespace Gui
{

/// Stops a combo box or spin box inside a scrolling list from taking wheel events unless it
/// has focus, so that the wheel scrolls the list instead of changing the value under it.
GuiExport void ignoreWheelUnlessFocused(QWidget* widget);

struct LayerStyle
{
    Base::Color color;
    /// A 16-bit stipple pattern, as used by Coin: 0xffff is solid.
    unsigned int pattern = 0xffff;
    /// In screen pixels.
    double lineWidth = 2.0;
};

struct LayerInfo
{
    /// Stable and unique among the layers of a model; used to address the layer.
    std::string id;
    std::string name;
    LayerStyle style;
    bool visible = true;
    bool locked = false;
    bool active = false;
    /// False for a layer that must always exist, such as a default layer.
    bool removable = true;
};

/// A per-layer setting that a model adds to the built-in ones (visibility, lock, color,
/// pattern, line width). The widget shows it in the layer's expanded settings, and on the
/// "Add Layer" row as a default for new layers, stored under `key` in the defaults group.
struct LayerSetting
{
    enum class Type
    {
        Bool,
        Double,
        Choice
    };
    std::string key;
    Type type = Type::Bool;
    QString label;
    QString toolTip;
    QString defaultsLabel;
    QString defaultsToolTip;
    /// Used when the defaults group has no value for the key.
    QVariant defaultValue;
    /// Double: range and suffix. Choice: the items; the value is the chosen index.
    double minimum = 0.0;
    double maximum = 100.0;
    QString suffix;
    QStringList choices;
    /// The key of a Bool setting that must be on for this one to be editable.
    std::string dependsOn;
};

/// Everything the layer widget knows about the layers it shows. A workbench implements
/// this for its own notion of a layer; the widget never touches documents directly, except
/// to wrap each change in a transaction on document().
class GuiExport LayerModel
{
public:
    enum class Feature
    {
        Add,
        Remove,
        Rename,
        Reorder,
        Activate,
        Lock,
        Style
    };

    virtual ~LayerModel();

    /// In display order.
    virtual std::vector<LayerInfo> layers() const = 0;
    virtual bool hasFeature(Feature) const
    {
        return true;
    }
    /// The document to open undo transactions on, or nullptr for none.
    virtual App::Document* document() const = 0;

    /// Adds a layer and makes it the active one.
    virtual void addLayer(const std::string& name) = 0;
    virtual void renameLayer(const std::string& id, const std::string& name) = 0;
    /// Asks what to do with the layer's contents, if anything. Returning false cancels.
    virtual bool prepareRemoveLayer(const std::string& id, QWidget* parent);
    virtual void removeLayer(const std::string& id) = 0;
    virtual void setActiveLayer(const std::string& id) = 0;
    virtual void setLayerVisible(const std::string& id, bool visible) = 0;
    virtual void setLayerLocked(const std::string& id, bool locked) = 0;
    virtual void setLayerColor(const std::string& id, const Base::Color& color) = 0;
    virtual void setLayerPattern(const std::string& id, unsigned int pattern) = 0;
    virtual void setLayerLineWidth(const std::string& id, double width) = 0;
    virtual void reorderLayers(const std::vector<std::string>& ids) = 0;

    virtual std::vector<LayerSetting> extraSettings() const;
    virtual QVariant setting(const std::string& id, const std::string& key) const;
    virtual void setSetting(const std::string& id, const std::string& key, const QVariant& value);

    /// Adds model-specific actions at the top of a layer's context menu.
    virtual void populateContextMenu(QMenu* menu, const std::string& id);

    /// Holds the defaults for new layers: Color ("#rrggbb"), Pattern, LineWidth and the
    /// keys of extraSettings(). Applying them to new layers is up to the model.
    virtual ParameterGrp::handle defaultsGroup() const = 0;
    /// The style of new layers where the defaults group has no value.
    virtual LayerStyle fallbackStyle() const;

    /// Emit when layers change outside the widget; the widget refreshes after the event.
    fastsignals::signal<void()> signalChanged;
};

/// A list of layers with visibility, lock, active layer, name, style and model-specific
/// settings. The widget owns its model.
class GuiExport LayerWidget: public QWidget
{
    Q_OBJECT

public:
    explicit LayerWidget(std::unique_ptr<LayerModel> model, QWidget* parent = nullptr);
    ~LayerWidget() override;

    LayerModel* model() const
    {
        return layerModel.get();
    }
    QTreeWidget* tree() const
    {
        return list;
    }
    /// Discards a layer name being typed on the "Add Layer" row.
    void cancelAddingLayer();

    static QString patternName(unsigned int pattern);

public Q_SLOTS:
    void refresh();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    const LayerInfo* findLayer(const std::string& id) const;
    void scheduleRefresh();
    void updateHeight();
    void addLayer();
    void finishAddingLayer(bool accept);
    void removeLayer(const std::string& id);
    void hideOtherLayers(const std::string& id);
    void chooseColor(const std::string& id, bool defaults);
    void addSettings(QTreeWidgetItem* item, const LayerInfo* layer);
    void addExtraSetting(QTreeWidgetItem* item, const LayerInfo* layer, const LayerSetting& setting);
    QVariant defaultSetting(const LayerSetting& setting) const;
    LayerStyle defaultStyle() const;
    QComboBox* patternPicker(const LayerInfo* layer);
    QDoubleSpinBox* widthPicker(const LayerInfo* layer);
    void applyChange(const char* label, const std::function<void()>& change);

    std::unique_ptr<LayerModel> layerModel;
    std::vector<LayerInfo> layers;
    QTreeWidget* list;
    QPointer<QLineEdit> newLayerEditor;
    fastsignals::scoped_connection connection;
    bool refreshPending = false;
};

}  // namespace Gui
