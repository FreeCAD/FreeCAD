// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>

#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QDrag>
#include <QDropEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolTip>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <App/Document.h>
#include <Base/Exception.h>

#include "BitmapFactory.h"
#include "LayerWidget.h"

using namespace Gui;

namespace
{
constexpr int NameColumn = 1;
constexpr int ValueColumn = 2;
constexpr int LockColumn = 3;
constexpr int IdRole = Qt::UserRole;
constexpr int SettingRole = Qt::UserRole + 1;
constexpr int AddRowRole = Qt::UserRole + 2;

// Keys of the built-in defaults, shared with models that apply them to new layers.
constexpr const char* ColorKey = "Color";
constexpr const char* PatternKey = "Pattern";
constexpr const char* LineWidthKey = "LineWidth";

const std::array<std::pair<const char*, unsigned int>, 4> patterns {
    {{QT_TRANSLATE_NOOP("Gui::LayerWidget", "Solid"), 0xffff},
     {QT_TRANSLATE_NOOP("Gui::LayerWidget", "Dashed"), 0xf0f0},
     {QT_TRANSLATE_NOOP("Gui::LayerWidget", "Dotted"), 0xaaaa},
     {QT_TRANSLATE_NOOP("Gui::LayerWidget", "Dash-dot"), 0xe4e4}}
};

class WheelGuard: public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (event->type() != QEvent::Wheel || !widget || widget->hasFocus()) {
            return false;
        }
        if (auto* parent = widget->parentWidget()) {
            QCoreApplication::sendEvent(parent, event);
        }
        return true;
    }
};

std::string layerId(const QTreeWidgetItem* item)
{
    return item->data(NameColumn, IdRole).toString().toStdString();
}

bool isAddRow(const QTreeWidgetItem* item)
{
    return item->data(NameColumn, AddRowRole).toBool();
}

bool isLayerRow(const QTreeWidgetItem* item)
{
    return item && !item->parent() && !isAddRow(item);
}

QIcon colorIcon(const Base::Color& color)
{
    QPixmap image(16, 16);
    image.fill(QColor::fromRgbF(color.r, color.g, color.b));
    QPainter painter(&image);
    painter.setPen(QColor(100, 100, 100));
    painter.drawRect(0, 0, 15, 15);
    QIcon icon(image);
    icon.addPixmap(image, QIcon::Selected);
    icon.addPixmap(image, QIcon::Active);
    return icon;
}

// Only the layer name can be edited; settings use their own controls.
class LayerDelegate: public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    bool renamable = true;
    QWidget* createEditor(
        QWidget* parent,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override
    {
        if (!renamable || index.column() != NameColumn || index.parent().isValid()) {
            return nullptr;
        }
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    QRect iconRect(const QModelIndex& index, const QRect& rect, QWidget* widget) const
    {
        QStyleOptionViewItem option;
        option.initFrom(widget);
        option.rect = rect;
        option.decorationSize = QSize(16, 16);
        initStyleOption(&option, index);
        return widget->style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &option, widget);
    }
};

// Keep drops flat and local to this list, with the creation row pinned at the end.
class LayerList: public QTreeWidget
{
public:
    using QTreeWidget::QTreeWidget;
    std::function<void()> reordered;
    std::function<void(const std::string&)> colorRequested;

protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        const auto index = indexAt(event->position().toPoint());
        auto* item = itemFromIndex(index);
        if (event->button() == Qt::LeftButton && colorRequested && isLayerRow(item)
            && index.column() == NameColumn
            && static_cast<LayerDelegate*>(itemDelegate())
                   ->iconRect(index, visualRect(index), this)
                   .contains(event->position().toPoint())) {
            const auto id = layerId(item);
            QTreeWidget::mouseReleaseEvent(event);
            // Finish the view's mouse handling before opening a modal editor.
            QTimer::singleShot(0, this, [this, id]() {
                if (colorRequested) {
                    colorRequested(id);
                }
            });
            event->accept();
            return;
        }
        QTreeWidget::mouseReleaseEvent(event);
    }
    void startDrag(Qt::DropActions) override
    {
        const auto items = selectedItems();
        if (items.size() != 1 || !isLayerRow(items.front())) {
            return;
        }
        // Own the move: the default item-view drag removes the source after a move drop.
        QDrag drag(this);
        drag.setMimeData(mimeData(items));
        drag.setPixmap(items.front()->icon(NameColumn).pixmap(24, 24));
        drag.exec(Qt::MoveAction);
    }
    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (draggedItem(event->mimeData())) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else {
            event->ignore();
        }
    }
    QStringList mimeTypes() const override
    {
        return {QStringLiteral("application/x-freecad-layer")};
    }
    QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override
    {
        auto* data = new QMimeData;
        if (items.size() == 1 && isLayerRow(items.front())) {
            data->setData(
                mimeTypes().front(),
                token() + items.front()->data(NameColumn, IdRole).toString().toUtf8()
            );
        }
        return data;
    }
    void dragMoveEvent(QDragMoveEvent* event) override
    {
        QTreeWidget::dragMoveEvent(event);
        if (draggedItem(event->mimeData())) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }
        else {
            event->ignore();
        }
    }
    void dropEvent(QDropEvent* event) override
    {
        auto* item = draggedItem(event->mimeData());
        if (!item) {
            event->ignore();
            return;
        }
        auto* target = itemAt(event->position().toPoint());
        while (target && target->parent()) {
            target = target->parent();
        }
        int destination = topLevelItemCount() - 1;
        if (target) {
            destination = indexOfTopLevelItem(target);
            if (event->position().y() > visualItemRect(target).center().y()) {
                ++destination;
            }
        }
        destination = std::min(destination, topLevelItemCount() - 1);
        const int source = indexOfTopLevelItem(item);
        if (source < destination) {
            --destination;
        }
        if (source != destination) {
            const bool expanded = item->isExpanded();
            takeTopLevelItem(source);
            insertTopLevelItem(destination, item);
            item->setExpanded(expanded);
            setCurrentItem(item, NameColumn);
            if (reordered) {
                reordered();
            }
        }
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }

private:
    // Only accept drags that started in this very list.
    QByteArray token() const
    {
        return QByteArray::number(reinterpret_cast<quintptr>(this)) + ':';
    }
    QTreeWidgetItem* draggedItem(const QMimeData* data) const
    {
        const auto bytes = data->data(mimeTypes().front());
        const auto prefix = token();
        if (!bytes.startsWith(prefix)) {
            return nullptr;
        }
        const auto id = QString::fromUtf8(bytes.mid(prefix.size()));
        for (int row = 0; row < topLevelItemCount(); ++row) {
            auto* item = topLevelItem(row);
            if (isLayerRow(item) && item->data(NameColumn, IdRole).toString() == id) {
                return item;
            }
        }
        return nullptr;
    }
};
}  // namespace

void Gui::ignoreWheelUnlessFocused(QWidget* widget)
{
    widget->setFocusPolicy(Qt::StrongFocus);
    widget->installEventFilter(new WheelGuard(widget));
}

// ----------------------------------------------------------------------------

LayerModel::~LayerModel() = default;

bool LayerModel::prepareRemoveLayer(const std::string&, QWidget*)
{
    return true;
}

std::vector<LayerSetting> LayerModel::extraSettings() const
{
    return {};
}

QVariant LayerModel::setting(const std::string&, const std::string&) const
{
    return {};
}

void LayerModel::setSetting(const std::string&, const std::string&, const QVariant&)
{}

void LayerModel::populateContextMenu(QMenu*, const std::string&)
{}

LayerStyle LayerModel::fallbackStyle() const
{
    LayerStyle style;
    style.color = Base::Color(1.0F, 1.0F, 1.0F);
    return style;
}

// ----------------------------------------------------------------------------

LayerWidget::LayerWidget(std::unique_ptr<LayerModel> model, QWidget* parent)
    : QWidget(parent)
    , layerModel(std::move(model))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* layerList = new LayerList(this);
    list = layerList;
    list->setObjectName(QStringLiteral("layers"));
    auto* delegate = new LayerDelegate(list);
    delegate->renamable = layerModel->hasFeature(LayerModel::Feature::Rename);
    list->setItemDelegate(delegate);
    if (layerModel->hasFeature(LayerModel::Feature::Style)) {
        layerList->colorRequested = [this](const std::string& id) {
            chooseColor(id, false);
        };
    }
    if (layerModel->hasFeature(LayerModel::Feature::Reorder)) {
        list->setDragDropMode(QAbstractItemView::InternalMove);
        list->setDefaultDropAction(Qt::MoveAction);
        list->setDropIndicatorShown(true);
        layerList->reordered = [this]() {
            std::vector<std::string> order;
            for (int row = 0; row < list->topLevelItemCount(); ++row) {
                if (isLayerRow(list->topLevelItem(row))) {
                    order.push_back(layerId(list->topLevelItem(row)));
                }
            }
            applyChange(QT_TRANSLATE_NOOP("Command", "Reorder layers"), [this, order]() {
                layerModel->reorderLayers(order);
            });
        };
    }
    list->setColumnCount(4);
    list->setHeaderHidden(true);
    list->setRootIsDecorated(true);
    list->setTreePosition(NameColumn);
    list->setExpandsOnDoubleClick(false);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setEditTriggers(QAbstractItemView::EditKeyPressed);
    list->header()->setStretchLastSection(false);
    list->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    list->header()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
    list->header()->setSectionResizeMode(ValueColumn, QHeaderView::ResizeToContents);
    list->header()->setSectionResizeMode(LockColumn, QHeaderView::ResizeToContents);
    // Keep the name/settings model columns stable and place the lock beside the eye.
    list->header()->moveSection(LockColumn, 1);
    list->setColumnHidden(LockColumn, !layerModel->hasFeature(LayerModel::Feature::Lock));
    list->setMinimumHeight(100);
    list->setMaximumHeight(320);
    connect(list, &QTreeWidget::itemExpanded, this, [this]() { updateHeight(); });
    connect(list, &QTreeWidget::itemCollapsed, this, [this]() { updateHeight(); });
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(list);
    list->installEventFilter(this);

    connect(list, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        if (item->parent()) {
            return;
        }
        if (isAddRow(item)) {
            addLayer();
            return;
        }
        const auto id = layerId(item);
        const auto* layer = findLayer(id);
        if (!layer) {
            return;
        }
        if (column == 0) {
            const bool visible = !layer->visible;
            applyChange(QT_TRANSLATE_NOOP("Command", "Toggle layer visibility"), [this, id, visible]() {
                layerModel->setLayerVisible(id, visible);
            });
        }
        else if (column == LockColumn) {
            const bool locked = !layer->locked;
            // Locking may process GUI events. Finish the click notification first.
            QTimer::singleShot(0, this, [this, id, locked]() {
                applyChange(QT_TRANSLATE_NOOP("Command", "Toggle layer lock"), [this, id, locked]() {
                    layerModel->setLayerLocked(id, locked);
                });
            });
        }
        else if (!layer->active && layerModel->hasFeature(LayerModel::Feature::Activate)) {
            applyChange(QT_TRANSLATE_NOOP("Command", "Activate layer"), [this, id]() {
                layerModel->setActiveLayer(id);
            });
        }
        // Keep keyboard editing on the name column and restore the active highlight after eye clicks.
        scheduleRefresh();
    });
    connect(list, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) {
        if (item->parent()) {
            if (column == ValueColumn && !item->data(NameColumn, SettingRole).isNull()) {
                const auto key = item->data(NameColumn, SettingRole).toString().toStdString();
                const bool checked = item->checkState(ValueColumn) == Qt::Checked;
                if (isAddRow(item->parent())) {
                    layerModel->defaultsGroup()->SetBool(key.c_str(), checked);
                    scheduleRefresh();
                    return;
                }
                const auto id = layerId(item->parent());
                // Solving may process GUI events. Finish the item-change notification first.
                QTimer::singleShot(0, this, [this, id, key, checked]() {
                    applyChange(
                        QT_TRANSLATE_NOOP("Command", "Change layer setting"),
                        [this, id, key, checked]() { layerModel->setSetting(id, key, checked); }
                    );
                });
            }
            return;
        }
        if (column != NameColumn || !isLayerRow(item)) {
            return;
        }
        const auto id = layerId(item);
        const auto name = item->text(NameColumn).trimmed().toStdString();
        applyChange(QT_TRANSLATE_NOOP("Command", "Rename layer"), [this, id, name]() {
            layerModel->renameLayer(id, name);
        });
    });
    connect(list, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& point) {
        auto* item = list->itemAt(point);
        if (!isLayerRow(item)) {
            return;
        }
        const auto id = layerId(item);
        const auto* layer = findLayer(id);
        if (!layer) {
            return;
        }
        const bool removable = layer->removable && !layer->locked;
        list->setCurrentItem(item, NameColumn);
        QMenu menu;
        layerModel->populateContextMenu(&menu, id);
        if (!menu.isEmpty()) {
            menu.addSeparator();
        }
        menu.addAction(tr("Hide Other Layers"), this, [this, id]() { hideOtherLayers(id); });
        if (layerModel->hasFeature(LayerModel::Feature::Rename)
            || layerModel->hasFeature(LayerModel::Feature::Remove)) {
            menu.addSeparator();
        }
        if (layerModel->hasFeature(LayerModel::Feature::Rename)) {
            auto* rename = menu.addAction(tr("Rename"), this, [this, id]() {
                for (int row = 0; row < list->topLevelItemCount(); ++row) {
                    auto* current = list->topLevelItem(row);
                    if (isLayerRow(current) && layerId(current) == id) {
                        list->editItem(current, NameColumn);
                        break;
                    }
                }
            });
            rename->setShortcut(QKeySequence(Qt::Key_F2));
        }
        if (layerModel->hasFeature(LayerModel::Feature::Remove)) {
            auto* remove = menu.addAction(tr("Remove Layer"), this, [this, id]() { removeLayer(id); });
            remove->setShortcut(QKeySequence(Qt::Key_Delete));
            remove->setEnabled(removable);
        }
        menu.exec(list->viewport()->mapToGlobal(point));
    });
    // Property changes can occur inside itemChanged; don't delete the item on that stack.
    connection = layerModel->signalChanged.connect([this]() { scheduleRefresh(); });
    refresh();
}

LayerWidget::~LayerWidget() = default;

QString LayerWidget::patternName(unsigned int pattern)
{
    for (const auto& [label, value] : patterns) {
        if (value == pattern) {
            return QCoreApplication::translate("Gui::LayerWidget", label);
        }
    }
    return QStringLiteral("0x%1").arg(pattern, 4, 16, QLatin1Char('0'));
}

const LayerInfo* LayerWidget::findLayer(const std::string& id) const
{
    auto it = std::find_if(layers.begin(), layers.end(), [&id](const auto& layer) {
        return layer.id == id;
    });
    return it == layers.end() ? nullptr : &*it;
}

void LayerWidget::scheduleRefresh()
{
    if (refreshPending) {
        return;
    }
    refreshPending = true;
    QTimer::singleShot(0, this, &LayerWidget::refresh);
}

void LayerWidget::cancelAddingLayer()
{
    finishAddingLayer(false);
}

void LayerWidget::refresh()
{
    refreshPending = false;
    if (newLayerEditor) {
        return;
    }
    QSignalBlocker blocker(list);
    QSet<QString> expanded;
    for (int row = 0; row < list->topLevelItemCount(); ++row) {
        auto* item = list->topLevelItem(row);
        if (item->isExpanded()) {
            expanded.insert(isAddRow(item) ? QString() : item->data(NameColumn, IdRole).toString());
        }
    }
    list->clear();
    layers = layerModel->layers();
    const bool styled = layerModel->hasFeature(LayerModel::Feature::Style);
    const bool renamable = layerModel->hasFeature(LayerModel::Feature::Rename);
    for (const auto& layer : layers) {
        const auto id = QString::fromStdString(layer.id);
        auto* item = new QTreeWidgetItem(list);
        item->setText(NameColumn, QString::fromStdString(layer.name));
        if (styled) {
            item->setIcon(NameColumn, colorIcon(layer.style.color));
        }
        item->setToolTip(
            NameColumn,
            renamable ? tr("New objects go to the active layer, shown in bold. "
                           "The color square changes the layer color; F2 renames the layer.")
                      : tr("New objects go to the active layer, shown in bold.")
        );
        item->setData(NameColumn, IdRole, id);
        auto flags = item->flags() & ~Qt::ItemIsDropEnabled;
        if (renamable) {
            flags |= Qt::ItemIsEditable;
        }
        if (layerModel->hasFeature(LayerModel::Feature::Reorder)) {
            flags |= Qt::ItemIsDragEnabled;
        }
        item->setFlags(flags);
        item->setIcon(
            0,
            BitmapFactory().iconFromTheme(layer.visible ? "TreeItemVisible" : "TreeItemInvisible")
        );
        item->setToolTip(0, layer.visible ? tr("Hides the layer") : tr("Shows the layer"));
        item->setData(0, Qt::AccessibleTextRole, layer.visible ? tr("Visible") : tr("Hidden"));
        auto lockIcon = BitmapFactory().iconFromTheme(layer.locked ? "LayerLocked" : "LayerUnlocked");
        if (!layer.locked) {
            const auto size = list->iconSize().isValid() ? list->iconSize() : QSize(16, 16);
            const auto muted = lockIcon.pixmap(size, list->devicePixelRatioF(), QIcon::Disabled);
            lockIcon = QIcon(muted);
            lockIcon.addPixmap(muted, QIcon::Selected);
            lockIcon.addPixmap(muted, QIcon::Active);
        }
        item->setIcon(LockColumn, lockIcon);
        item->setToolTip(
            LockColumn,
            layer.locked ? tr("Unlocks the layer so that its contents can be edited")
                         : tr("Locks the layer so that its contents cannot be changed or deleted")
        );
        item->setData(LockColumn, Qt::AccessibleTextRole, layer.locked ? tr("Locked") : tr("Unlocked"));
        if (styled) {
            auto* rowPattern = patternPicker(&layer);
            rowPattern->setObjectName(QStringLiteral("layerRowPattern%1").arg(id));
            list->setItemWidget(item, ValueColumn, rowPattern);
        }
        addSettings(item, &layer);
        item->setExpanded(item->childCount() > 0 && expanded.contains(id));
        if (layer.active) {
            auto font = item->font(NameColumn);
            font.setBold(true);
            item->setFont(NameColumn, font);
            list->setCurrentItem(item, NameColumn);
        }
    }
    if (layerModel->hasFeature(LayerModel::Feature::Add)) {
        auto* add = new QTreeWidgetItem(list);
        add->setText(NameColumn, tr("Add Layer"));
        add->setIcon(0, BitmapFactory().iconFromTheme("list-add"));
        add->setData(NameColumn, AddRowRole, true);
        add->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        add->setToolTip(
            NameColumn,
            tr("Adds a layer and makes it active. The expanded row holds the defaults for new "
               "layers; they are preferences, not part of the document.")
        );
        addSettings(add, nullptr);
        add->setExpanded(add->childCount() > 0 && expanded.contains(QString()));
    }
    updateHeight();
}

LayerStyle LayerWidget::defaultStyle() const
{
    auto style = layerModel->fallbackStyle();
    const auto group = layerModel->defaultsGroup();
    const QColor preferred(QString::fromStdString(group->GetASCII(ColorKey, "")));
    if (preferred.isValid()) {
        style.color = Base::Color(preferred.redF(), preferred.greenF(), preferred.blueF());
    }
    style.pattern = static_cast<unsigned int>(
        group->GetInt(PatternKey, static_cast<long>(style.pattern))
    );
    style.lineWidth = group->GetFloat(LineWidthKey, style.lineWidth);
    return style;
}

QVariant LayerWidget::defaultSetting(const LayerSetting& setting) const
{
    const auto group = layerModel->defaultsGroup();
    const char* key = setting.key.c_str();
    switch (setting.type) {
        case LayerSetting::Type::Bool:
            return group->GetBool(key, setting.defaultValue.toBool());
        case LayerSetting::Type::Double:
            return group->GetFloat(key, setting.defaultValue.toDouble());
        case LayerSetting::Type::Choice:
            return static_cast<int>(group->GetInt(key, setting.defaultValue.toInt()));
    }
    return {};
}

// A null layer is the "Add Layer" row: its settings are the defaults for new layers.
void LayerWidget::addSettings(QTreeWidgetItem* item, const LayerInfo* layer)
{
    const bool defaults = !layer;
    const auto suffix = defaults ? QStringLiteral("Defaults") : QString::fromStdString(layer->id);
    if (layerModel->hasFeature(LayerModel::Feature::Style)) {
        const auto color = defaults ? defaultStyle().color : layer->style.color;
        auto* colorItem = new QTreeWidgetItem(item);
        colorItem->setText(NameColumn, defaults ? tr("Default color") : tr("Color"));
        colorItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        auto* colorButton = new QPushButton(colorIcon(color), tr("Choose…"), list);
        colorButton->setObjectName(QStringLiteral("layerColor%1").arg(suffix));
        colorButton->setToolTip(
            defaults ? tr("Sets the color of new layers") : tr("Sets the color of the layer")
        );
        const auto id = defaults ? std::string() : layer->id;
        connect(colorButton, &QPushButton::clicked, this, [this, id, defaults]() {
            chooseColor(id, defaults);
        });
        list->setItemWidget(colorItem, ValueColumn, colorButton);
        auto* pattern = new QTreeWidgetItem(item);
        pattern->setText(NameColumn, defaults ? tr("Default pattern") : tr("Pattern"));
        pattern->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        list->setItemWidget(pattern, ValueColumn, patternPicker(layer));
        auto* thickness = new QTreeWidgetItem(item);
        thickness->setText(NameColumn, defaults ? tr("Default line thickness") : tr("Line thickness"));
        const auto thicknessTip = tr("Sets the line width in screen pixels, independent of zoom");
        thickness->setToolTip(NameColumn, thicknessTip);
        thickness->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        auto* width = widthPicker(layer);
        width->setToolTip(thicknessTip);
        list->setItemWidget(thickness, ValueColumn, width);
    }
    for (const auto& setting : layerModel->extraSettings()) {
        addExtraSetting(item, layer, setting);
    }
}

void LayerWidget::addExtraSetting(QTreeWidgetItem* item, const LayerInfo* layer, const LayerSetting& setting)
{
    const bool defaults = !layer;
    auto valueOf = [&](const std::string& key) {
        if (!defaults) {
            return layerModel->setting(layer->id, key);
        }
        const auto settings = layerModel->extraSettings();
        auto it = std::find_if(settings.begin(), settings.end(), [&key](const auto& s) {
            return s.key == key;
        });
        return it == settings.end() ? QVariant() : defaultSetting(*it);
    };
    const bool enabled = setting.dependsOn.empty() || valueOf(setting.dependsOn).toBool();
    const auto value = valueOf(setting.key);
    const auto toolTip = defaults && !setting.defaultsToolTip.isEmpty() ? setting.defaultsToolTip
                                                                        : setting.toolTip;
    const auto key = QString::fromStdString(setting.key);
    const auto suffix = defaults ? QStringLiteral("Defaults") : QString::fromStdString(layer->id);

    auto* child = new QTreeWidgetItem(item);
    child->setText(
        NameColumn,
        defaults && !setting.defaultsLabel.isEmpty() ? setting.defaultsLabel : setting.label
    );
    child->setToolTip(NameColumn, toolTip);
    child->setToolTip(ValueColumn, toolTip);
    child->setFlags(Qt::ItemIsSelectable | (enabled ? Qt::ItemIsEnabled : Qt::NoItemFlags));

    // Stores a new value, either as a default or on the layer, after the editor's own event.
    auto store = [this,
                  defaults,
                  id = defaults ? std::string() : layer->id,
                  stdKey = setting.key,
                  type = setting.type](const QVariant& newValue) {
        if (defaults) {
            const auto group = layerModel->defaultsGroup();
            if (type == LayerSetting::Type::Double) {
                group->SetFloat(stdKey.c_str(), newValue.toDouble());
            }
            else {
                group->SetInt(stdKey.c_str(), newValue.toInt());
            }
            scheduleRefresh();
            return;
        }
        QTimer::singleShot(0, this, [this, id, stdKey, newValue]() {
            applyChange(
                QT_TRANSLATE_NOOP("Command", "Change layer setting"),
                [this, id, stdKey, newValue]() { layerModel->setSetting(id, stdKey, newValue); }
            );
        });
    };

    switch (setting.type) {
        case LayerSetting::Type::Bool:
            // Handled by the list's itemChanged connection.
            child->setFlags(child->flags() | Qt::ItemIsUserCheckable);
            child->setData(NameColumn, SettingRole, key);
            child->setCheckState(ValueColumn, value.toBool() ? Qt::Checked : Qt::Unchecked);
            break;
        case LayerSetting::Type::Double: {
            auto* spin = new QDoubleSpinBox(list);
            spin->setObjectName(QStringLiteral("layerSetting%1%2").arg(key, suffix));
            spin->setRange(setting.minimum, setting.maximum);
            spin->setSuffix(setting.suffix);
            spin->setKeyboardTracking(false);
            spin->setValue(value.toDouble());
            spin->setEnabled(enabled);
            spin->setToolTip(toolTip);
            ignoreWheelUnlessFocused(spin);
            connect(spin, &QDoubleSpinBox::valueChanged, this, [store](double v) { store(v); });
            list->setItemWidget(child, ValueColumn, spin);
            break;
        }
        case LayerSetting::Type::Choice: {
            auto* combo = new QComboBox(list);
            combo->setObjectName(QStringLiteral("layerSetting%1%2").arg(key, suffix));
            combo->addItems(setting.choices);
            combo->setCurrentIndex(value.toInt());
            combo->setEnabled(enabled);
            combo->setToolTip(toolTip);
            ignoreWheelUnlessFocused(combo);
            connect(combo, &QComboBox::activated, this, [store](int index) { store(index); });
            list->setItemWidget(child, ValueColumn, combo);
            break;
        }
    }
}

void LayerWidget::updateHeight()
{
    int rows = list->topLevelItemCount();
    for (int row = 0; row < list->topLevelItemCount(); ++row) {
        auto* item = list->topLevelItem(row);
        if (item->isExpanded()) {
            rows += item->childCount();
        }
    }
    const int rowHeight = std::max(24, list->sizeHintForRow(0));
    list->setMinimumHeight(std::clamp(rows * rowHeight + 2, 100, 320));
}

void LayerWidget::applyChange(const char* label, const std::function<void()>& change)
{
    auto* document = layerModel->document();
    if (document) {
        document->openTransaction(label);
    }
    try {
        change();
        if (document) {
            document->commitTransaction();
        }
    }
    // Commands report Python errors as Base::PyException, which is not a std::exception.
    catch (const Base::Exception& e) {
        if (document) {
            document->abortTransaction();
        }
        QMessageBox::warning(this, tr("Layers"), QString::fromUtf8(e.what()));
    }
    catch (const std::exception& e) {
        if (document) {
            document->abortTransaction();
        }
        QMessageBox::warning(this, tr("Layers"), QString::fromUtf8(e.what()));
    }
    scheduleRefresh();
}

void LayerWidget::addLayer()
{
    if (newLayerEditor) {
        newLayerEditor->setFocus();
        return;
    }
    int number = 1;
    QString name;
    do {
        name = tr("Layer%1").arg(number++);
    } while (std::any_of(layers.begin(), layers.end(), [&name](const auto& layer) {
        return layer.name == name.toStdString();
    }));
    auto* item = list->topLevelItem(list->topLevelItemCount() - 1);
    list->setCurrentItem(item, NameColumn);
    list->scrollToItem(item);
    newLayerEditor = new QLineEdit(name, list);
    newLayerEditor->setObjectName(QStringLiteral("newLayerName"));
    newLayerEditor->installEventFilter(this);
    list->setItemWidget(item, NameColumn, newLayerEditor);
    newLayerEditor->setFocus();
    newLayerEditor->selectAll();
}

void LayerWidget::finishAddingLayer(bool accept)
{
    if (!newLayerEditor) {
        return;
    }
    const QString name = newLayerEditor->text().trimmed();
    if (accept) {
        const bool duplicate = std::any_of(layers.begin(), layers.end(), [&name](const auto& layer) {
            return layer.name == name.toStdString();
        });
        if (name.isEmpty() || duplicate) {
            QToolTip::showText(
                newLayerEditor->mapToGlobal(QPoint(0, newLayerEditor->height())),
                name.isEmpty() ? tr("A layer name cannot be empty")
                               : tr("A layer with this name already exists"),
                newLayerEditor
            );
            newLayerEditor->selectAll();
            return;
        }
        applyChange(QT_TRANSLATE_NOOP("Command", "Add layer"), [this, name]() {
            layerModel->addLayer(name.toStdString());
        });
    }
    newLayerEditor = nullptr;
    // The editor can be handling a key/focus event; destroy it after that event returns.
    scheduleRefresh();
}

void LayerWidget::removeLayer(const std::string& id)
{
    const auto* layer = findLayer(id);
    if (!layer || !layer->removable || layer->locked
        || !layerModel->hasFeature(LayerModel::Feature::Remove)) {
        return;
    }
    if (!layerModel->prepareRemoveLayer(id, this)) {
        return;
    }
    applyChange(QT_TRANSLATE_NOOP("Command", "Remove layer"), [this, id]() {
        layerModel->removeLayer(id);
    });
}

void LayerWidget::hideOtherLayers(const std::string& id)
{
    const auto current = layers;
    applyChange(QT_TRANSLATE_NOOP("Command", "Hide other layers"), [this, id, current]() {
        for (const auto& layer : current) {
            if (layer.visible != (layer.id == id)) {
                layerModel->setLayerVisible(layer.id, layer.id == id);
            }
        }
    });
}

void LayerWidget::chooseColor(const std::string& id, bool defaults)
{
    Base::Color color = defaultStyle().color;
    if (!defaults) {
        const auto* layer = findLayer(id);
        if (!layer) {
            return;
        }
        color = layer->style.color;
    }
    QColorDialog dialog(QColor::fromRgbF(color.r, color.g, color.b), this);
    dialog.setObjectName(QStringLiteral("layerColorDialog"));
    dialog.setWindowTitle(tr("Layer Color"));
    dialog.setOption(QColorDialog::DontUseNativeDialog);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const auto chosen = dialog.selectedColor();
    if (defaults) {
        layerModel->defaultsGroup()->SetASCII(ColorKey, chosen.name().toStdString().c_str());
        scheduleRefresh();
        return;
    }
    const Base::Color value(chosen.redF(), chosen.greenF(), chosen.blueF());
    applyChange(QT_TRANSLATE_NOOP("Command", "Change layer color"), [this, id, value]() {
        layerModel->setLayerColor(id, value);
    });
}

QDoubleSpinBox* LayerWidget::widthPicker(const LayerInfo* layer)
{
    const bool defaults = !layer;
    auto* picker = new QDoubleSpinBox(list);
    picker->setObjectName(
        QStringLiteral("layerLineWidth%1")
            .arg(defaults ? QStringLiteral("Defaults") : QString::fromStdString(layer->id))
    );
    picker->setRange(1, 64);
    picker->setDecimals(1);
    picker->setSingleStep(0.5);
    picker->setSuffix(tr(" px"));
    picker->setKeyboardTracking(false);
    ignoreWheelUnlessFocused(picker);
    picker->setValue(defaults ? defaultStyle().lineWidth : layer->style.lineWidth);
    const auto id = defaults ? std::string() : layer->id;
    connect(picker, &QDoubleSpinBox::valueChanged, this, [this, id, defaults](double value) {
        if (defaults) {
            layerModel->defaultsGroup()->SetFloat(LineWidthKey, value);
            return;
        }
        // Rebuilding the list must happen after the spin box's own event handling.
        QTimer::singleShot(0, this, [this, id, value]() {
            applyChange(
                QT_TRANSLATE_NOOP("Command", "Change layer line thickness"),
                [this, id, value]() { layerModel->setLayerLineWidth(id, value); }
            );
        });
    });
    return picker;
}

QComboBox* LayerWidget::patternPicker(const LayerInfo* layer)
{
    const bool defaults = !layer;
    auto* picker = new QComboBox(list);
    picker->setObjectName(
        QStringLiteral("layerPattern%1")
            .arg(defaults ? QStringLiteral("Defaults") : QString::fromStdString(layer->id))
    );
    for (const auto& [label, value] : patterns) {
        picker->addItem(QCoreApplication::translate("Gui::LayerWidget", label), value);
    }
    const auto current = defaults ? defaultStyle().pattern : layer->style.pattern;
    if (picker->findData(current) < 0) {
        // Keep a custom pattern set elsewhere selectable, instead of showing it as solid.
        picker->addItem(patternName(current), current);
    }
    picker->setToolTip(
        defaults ? tr("Sets the line pattern of new layers") : tr("Sets the line pattern of the layer")
    );
    ignoreWheelUnlessFocused(picker);
    picker->setCurrentIndex(picker->findData(current));
    const auto id = defaults ? std::string() : layer->id;
    connect(picker, &QComboBox::activated, this, [this, id, defaults, picker](int) {
        const auto pattern = picker->currentData().toUInt();
        if (defaults) {
            layerModel->defaultsGroup()->SetInt(PatternKey, static_cast<long>(pattern));
            return;
        }
        applyChange(QT_TRANSLATE_NOOP("Command", "Change layer pattern"), [this, id, pattern]() {
            layerModel->setLayerPattern(id, pattern);
        });
    });
    return picker;
}

bool LayerWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (newLayerEditor && watched == newLayerEditor) {
        if (event->type() == QEvent::FocusOut) {
            QTimer::singleShot(0, this, [this]() {
                if (newLayerEditor && !newLayerEditor->hasFocus()) {
                    finishAddingLayer(false);
                }
            });
        }
        if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress) {
            auto* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter
                || key->key() == Qt::Key_Escape) {
                event->accept();
                if (event->type() == QEvent::KeyPress && !key->isAutoRepeat()) {
                    finishAddingLayer(key->key() != Qt::Key_Escape);
                }
                return true;
            }
        }
    }
    if (watched == list && list->hasFocus()
        && (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)) {
        auto* key = static_cast<QKeyEvent*>(event);
        auto* item = list->currentItem();
        const bool topLevel = item && !item->parent();
        const bool remove = key->key() == Qt::Key_Delete;
        const bool add = topLevel && isAddRow(item)
            && (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter
                || key->key() == Qt::Key_F2);
        if ((remove || add) && key->modifiers() == Qt::NoModifier) {
            // Claim Delete before the application's delete shortcut sees it.
            event->accept();
            if (event->type() == QEvent::KeyPress && !key->isAutoRepeat()) {
                if (add) {
                    addLayer();
                }
                else if (isLayerRow(item) && item->isSelected()) {
                    removeLayer(layerId(item));
                }
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

#include "moc_LayerWidget.cpp"
