// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include <algorithm>
#include <functional>
#include <QComboBox>
#include <QCoreApplication>
#include <Gui/LayerWidget.h>
#include <QListWidget>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTimer>
#include <Mod/Sketcher/App/SketchObject.h>
#include "ViewProviderSketch.h"

namespace SketcherGui
{
class LayerListWidget: public QListWidget
{
public:
    using QListWidget::QListWidget;

protected:
    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override
    {
        if (event && isPersistentEditorOpen(itemFromIndex(index))) {
            // Qt otherwise forwards every event in the row to its persistent editor,
            // even outside the layer combobox, swallowing checkbox and selection clicks.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            auto option = viewOptions();
#else
            QStyleOptionViewItem option;
            initViewItemOption(&option);
#endif
            option.rect = visualRect(index);
            return itemDelegate(index)->editorEvent(event, model(), option, index);
        }
        return QListWidget::edit(index, trigger, event);
    }
};

// A real, persistent layer selector at the right of a single-column list row.
class LayerListDelegate: public QStyledItemDelegate
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::LayerListDelegate)

public:
    static constexpr int LayerRole = Qt::UserRole + 20;
    static constexpr int LayerEditableRole = Qt::UserRole + 21;
    using ChangeLayer = std::function<void(const QModelIndex&, int)>;
    LayerListDelegate(QObject* parent, ViewProviderSketch* view, ChangeLayer change)
        : QStyledItemDelegate(parent)
        , view(view)
        , change(std::move(change))
    {}
    void setView(ViewProviderSketch* value)
    {
        view = value;
    }
    void setLayersEnabled(bool value)
    {
        layersEnabled = value;
    }
    int layerWidth(const QStyleOptionViewItem& option) const
    {
        return layersEnabled
            ? std::min(
                  option.rect.width() / 3,
                  option.fontMetrics.horizontalAdvance(QStringLiteral("Default layer")) + 30
              )
            : 0;
    }
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        auto content = option;
        content.rect.adjust(0, 0, -layerWidth(option), 0);
        QStyledItemDelegate::paint(painter, content, index);
    }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        auto size = QStyledItemDelegate::sizeHint(option, index);
        if (layersEnabled) {
            size.setHeight(std::max(size.height(), option.fontMetrics.height() + 10));
        }
        return size;
    }
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const override
    {
        auto* combo = new QComboBox(parent);
        combo->setObjectName(QStringLiteral("sketchItemLayer"));
        combo->setToolTip(tr("Moves the item to another layer"));
        Gui::ignoreWheelUnlessFocused(combo);
        const QPersistentModelIndex persistent(index);
        connect(combo, qOverload<int>(&QComboBox::activated), combo, [this, combo, persistent](int row) {
            const int layer = combo->itemData(row).toInt();
            // Geometry changes may rebuild the list and destroy this editor.
            QTimer::singleShot(0, const_cast<LayerListDelegate*>(this), [this, persistent, layer] {
                if (persistent.isValid()) {
                    change(persistent, layer);
                }
            });
        });
        return combo;
    }
    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto* combo = static_cast<QComboBox*>(editor);
        QSignalBlocker block(combo);
        const auto layers = view->getSketchObject()->getLayers();
        bool rebuild = combo->count() != static_cast<int>(layers.size());
        int row = 0;
        for (const auto& [id, name] : layers) {
            rebuild |= combo->itemData(row++).toInt() != id;
        }
        if (rebuild) {
            combo->clear();
            for (const auto& [id, name] : layers) {
                combo->addItem(QString::fromStdString(name), id);
            }
        }
        row = 0;
        for (const auto& [id, name] : layers) {
            combo->setItemText(row, QString::fromStdString(name));
            static_cast<QStandardItemModel*>(combo->model())
                ->item(row++)
                ->setEnabled(!view->getSketchObject()->isLayerLocked(id));
        }
        combo->setCurrentIndex(combo->findData(index.data(LayerRole)));
        combo->setEnabled(index.data(LayerEditableRole).toBool());
    }
    void setModelData(QWidget*, QAbstractItemModel*, const QModelIndex&) const override
    {}
    void updateEditorGeometry(
        QWidget* editor,
        const QStyleOptionViewItem& option,
        const QModelIndex&
    ) const override
    {
        auto rect = option.rect;
        rect.setLeft(rect.right() - layerWidth(option) + 1);
        editor->setGeometry(rect.adjusted(2, 1, -1, -1));
    }

protected:
    ViewProviderSketch* view;
    ChangeLayer change;
    bool layersEnabled = false;
};
}  // namespace SketcherGui
