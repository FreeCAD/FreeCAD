// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "Tools.h"
#include "Application.h"
#include "OverlayManager.h"
#include "DlgThemeEditor.h"
#include "ui_DlgThemeEditor.h"
#include "BitmapFactory.h"

#include <Utilities.h>
#include <Base/ServiceProvider.h>
#include <Base/Tools.h>

#include <ranges>
#include <QImageReader>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTimer>

QPixmap colorPreview(const QColor& color)
{
    constexpr qsizetype size = 16;

    QPixmap preview = Gui::BitmapFactory().empty({size, size});

    QPainter painter(&preview);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QRect {0, 0, size, size});

    return preview;
}

QString typeOfTokenValue(const Gui::StyleParameters::Value& value)
{
    // clang-format off
    return std::visit(
        Base::Overloads {
            [](const std::string&) {
                return QWidget::tr("Generic");
            },
            [](const Gui::StyleParameters::Numeric&) {
                return QWidget::tr("Numeric");
            },
            [](const Base::Color&) {
                return QWidget::tr("Color");
            },
            [](const Gui::StyleParameters::Tuple&) {
                return QWidget::tr("Tuple");
            }
        },
        value
    );
    // clang-format on
}

namespace Gui
{
struct StyleParametersModel::Item
{
    Item() = default;
    virtual ~Item() = default;

    FC_DEFAULT_COPY_MOVE(Item);

    virtual bool isHeader() const = 0;
};

struct StyleParametersModel::GroupItem: Item
{
    explicit GroupItem(QString title, ParameterSource* source)
        : title(std::move(title))
        , canAddNewParameters(source && source->metadata.options.testFlag(UserEditable))
        , source(source)
    {}

    bool isHeader() const override
    {
        return true;
    }

    QString title;
    bool canAddNewParameters {false};
    ParameterSource* source;
    std::set<std::string> deleted {};
};

struct StyleParametersModel::ParameterItem: Item
{
    ParameterItem(QString name, StyleParameters::Parameter token)
        : name(std::move(name))
        , token(std::move(token))
    {}

    bool isHeader() const override
    {
        return false;
    }

    QString name;
    StyleParameters::Parameter token;
    QFlags<Qt::ItemFlag> flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
};

class StyleParametersModel::Node
{
public:
    explicit Node(std::unique_ptr<Item> data, Node* parent = nullptr)
        : _parent(parent)
        , _data(std::move(data))
    {}

    void appendChild(std::unique_ptr<Node> child)
    {
        child->_parent = this;
        _children.push_back(std::move(child));
    }

    void removeChild(const int row)
    {
        if (row >= 0 && row < static_cast<int>(_children.size())) {
            _children.erase(_children.begin() + row);
        }
    }

    Node* child(const int row) const
    {
        if (row < 0 || row >= static_cast<int>(_children.size())) {
            if (!_empty) {
                _empty = std::make_unique<Node>(nullptr, const_cast<Node*>(this));
            }

            return _empty.get();
        }

        return _children[row].get();
    }

    int childCount() const
    {
        return static_cast<int>(_children.size());
    }

    int row() const
    {
        if (!_parent) {
            return 0;
        }

        const auto& siblings = _parent->_children;
        for (size_t i = 0; i < siblings.size(); ++i) {
            if (siblings[i].get() == this) {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    Item* data() const
    {
        return _data.get();
    }

    template<class T>
    T* data() const
    {
        return dynamic_cast<T*>(_data.get());
    }

    Node* parent() const
    {
        return _parent;
    }

private:
    Node* _parent;
    std::vector<std::unique_ptr<Node>> _children {};

    mutable std::unique_ptr<Node> _empty {};
    std::unique_ptr<Item> _data {};
};

class DlgThemeEditor::Delegate: public QStyledItemDelegate
{
    Q_OBJECT

    QRegularExpression validNameRegExp {QStringLiteral("^[A-Z][a-zA-Z0-9]*$")};
    QRegularExpressionValidator* nameValidator;

public:
    explicit Delegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
        , nameValidator(new QRegularExpressionValidator(validNameRegExp, this))
    {}

    QWidget* createEditor(
        QWidget* parent,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override
    {
        auto model = dynamic_cast<const StyleParametersModel*>(index.model());
        if (!model) {
            return nullptr;
        }

        if (model->item<StyleParametersModel::ParameterItem>(index)
            && index.column() == StyleParametersModel::ParameterExpression) {
            return new QLineEdit(parent);
        }

        if (index.column() == StyleParametersModel::ParameterName) {
            auto editor = new QLineEdit(parent);
            editor->setValidator(nameValidator);
            return editor;
        }

        return nullptr;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        if (auto* lineEdit = qobject_cast<QLineEdit*>(editor)) {
            lineEdit->setText(index.data(Qt::DisplayRole).toString());
        }
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        if (auto* lineEdit = qobject_cast<QLineEdit*>(editor)) {
            model->setData(index, lineEdit->text(), Qt::EditRole);
        }
    }

    void updateEditorGeometry(
        QWidget* editor,
        const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index
    ) const override
    {
        editor->setGeometry(option.rect);
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        constexpr int height = 36;

        QSize base = QStyledItemDelegate::sizeHint(option, index);
        return {base.width(), std::max(base.height(), height)};
    }

    void paintAddPlaceholder(QPainter* painter, const QStyleOptionViewItem& option) const
    {
        QStyle* style = option.widget ? option.widget->style() : QApplication::style();
        QRect rect = style->subElementRect(QStyle::SE_ItemViewItemText, &option, option.widget);

        QFont font = option.font;
        font.setItalic(true);

        painter->setFont(font);
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, tr("New parameter..."));
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        auto model = dynamic_cast<const StyleParametersModel*>(index.model());

        painter->save();

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        if (model->isAddPlaceholder(index)) {
            if (index.column() == StyleParametersModel::ParameterName) {
                paintAddPlaceholder(painter, opt);
            }
        }
        else if (model->item<StyleParametersModel::GroupItem>(index)) {
            constexpr int headerContrast = 120;

            const bool isLightTheme = option.palette.color(QPalette::Text).lightness() < 128;

            const QColor headerBackgroundColor = QtTools::valueOr(
                option.widget->property("headerBackgroundColor"),
                isLightTheme ? option.palette.color(QPalette::AlternateBase).darker(headerContrast)
                             : option.palette.color(QPalette::AlternateBase).lighter(headerContrast)
            );

            painter->fillRect(option.rect, headerBackgroundColor);
            QStyledItemDelegate::paint(painter, option, index);
        }
        else {
            QStyledItemDelegate::paint(painter, option, index);
        }

        painter->restore();
    }
};

void TokenTreeView::keyPressEvent(QKeyEvent* event)
{
    static constexpr auto expressionEditKeys = {Qt::Key_Return, Qt::Key_Enter, Qt::Key_Space};
    static constexpr auto nameEditKeys = {Qt::Key_F2};
    static constexpr auto deleteKeys = {Qt::Key_Delete};

    const auto isCorrectKey = [&event](auto key) {
        return event->key() == key;
    };

    if (QModelIndex index = currentIndex(); index.isValid()) {
        if (std::ranges::any_of(expressionEditKeys, isCorrectKey)) {
            edit(index.siblingAtColumn(StyleParametersModel::ParameterExpression));
            return;
        }

        if (std::ranges::any_of(nameEditKeys, isCorrectKey)) {
            edit(index.siblingAtColumn(StyleParametersModel::ParameterName));
            return;
        }

        if (std::ranges::any_of(deleteKeys, isCorrectKey)) {
            requestRemove(currentIndex());
            return;
        }
    }

    QTreeView::keyPressEvent(event);
}

StyleParametersModel::StyleParametersModel(
    const std::list<StyleParameters::ParameterSource*>& sources,
    QObject* parent
)
    : QAbstractItemModel(parent)
    , ParameterSource({.name = QT_TR_NOOP("All Theme Editor Parameters")})
    , sources(sources)
    , manager(new StyleParameters::ParameterManager())
{
    // The parameter model serves as the source, so the manager can compute all necessary things
    manager->addSource(this);

    reset();
}

StyleParametersModel::~StyleParametersModel() = default;

std::list<StyleParameters::Parameter> StyleParametersModel::all() const
{
    std::map<std::string, StyleParameters::Parameter> result;

    QtTools::walkTreeModel(this, [this, &result](const QModelIndex& index) {
        if (auto parameterItem = item<ParameterItem>(index)) {
            if (result.contains(parameterItem->token.name)) {
                return;
            }

            result[parameterItem->token.name] = parameterItem->token;
        }
    });

    const auto values = result | std::ranges::views::values;
    return std::list<StyleParameters::Parameter>(values.begin(), values.end());
}

std::optional<StyleParameters::Parameter> StyleParametersModel::get(const std::string& name) const
{
    std::optional<StyleParameters::Parameter> result = std::nullopt;

    QtTools::walkTreeModel(this, [this, &name, &result](const QModelIndex& index) {
        if (auto parameterItem = item<ParameterItem>(index)) {
            if (parameterItem->token.name == name) {
                result = parameterItem->token;
                return true;
            }
        }

        return false;
    });

    return result;
}

void StyleParametersModel::removeItem(const QModelIndex& index)
{
    if (auto parameterItem = item<ParameterItem>(index)) {
        auto groupItem = item<GroupItem>(index.parent());

        if (!groupItem->source->metadata.options.testFlag(UserEditable)) {
            return;
        }

        groupItem->deleted.insert(parameterItem->token.name);

        beginRemoveRows(index.parent(), index.row(), index.row());
        node(index.parent())->removeChild(index.row());
        endRemoveRows();
    }
}

void StyleParametersModel::reset()
{
    using enum StyleParameters::ParameterSourceOption;

    beginResetModel();
    root = std::make_unique<Node>(std::make_unique<GroupItem>(tr("Root"), nullptr));

    for (auto* source : sources) {
        auto groupNode = std::make_unique<Node>(
            std::make_unique<GroupItem>(tr(source->metadata.name.c_str()), source)
        );

        for (const auto& parameter : source->all()) {
            auto item = std::make_unique<Node>(
                std::make_unique<ParameterItem>(QString::fromStdString(parameter.name), parameter)
            );

            if (source->metadata.options.testFlag(ReadOnly)) {
                item->data<ParameterItem>()->flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }

            groupNode->appendChild(std::move(item));
        }

        root->appendChild(std::move(groupNode));
    }

    endResetModel();
}

void StyleParametersModel::flush()
{
    QtTools::walkTreeModel(this, [this](const QModelIndex& index) {
        if (const auto& groupItem = item<GroupItem>(index)) {
            for (const auto& parameter : groupItem->deleted) {
                groupItem->source->remove(parameter);
            }

            groupItem->deleted.clear();
        }

        if (const auto& parameterItem = item<ParameterItem>(index)) {
            const auto& groupItem = item<GroupItem>(index.parent());

            groupItem->source->define(parameterItem->token);
        }
    });

    for (auto* source : sources) {
        source->flush();
    }

    reset();
}

int StyleParametersModel::rowCount(const QModelIndex& index) const
{
    if (index.column() > 0) {
        return 0;
    }

    int childCount = node(index)->childCount();

    if (const auto& groupItem = item<GroupItem>(index)) {
        return childCount + (groupItem->canAddNewParameters ? 1 : 0);
    }

    return childCount;
}

int StyleParametersModel::columnCount([[maybe_unused]] const QModelIndex& index) const
{
    return ColumnCount;
}

QVariant StyleParametersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case ParameterName:
                return tr("Name");
            case ParameterExpression:
                return tr("Expression");
            case ParameterPreview:
                return tr("Preview");
            case ParameterType:
                return tr("Type");
            default:
                return {};
        }
    }

    return {};
}

QVariant StyleParametersModel::data(const QModelIndex& index, int role) const
{
    if (auto parameterItem = item<ParameterItem>(index)) {
        const auto& [name, token, _] = *parameterItem;
        const auto& value = manager->resolve(name.toStdString());

        if (!value) {
            return {};
        }

        if (role == Qt::DisplayRole) {
            if (index.column() == ParameterName) {
                return name;
            }
            if (index.column() == ParameterExpression) {
                return QString::fromStdString(token.value);
            }
            if (index.column() == ParameterType) {
                return typeOfTokenValue(*value);
            }
            if (index.column() == ParameterPreview) {
                return QString::fromStdString(value->toString());
            }
        }

        if (role == Qt::DecorationRole) {
            if (index.column() == ParameterPreview && std::holds_alternative<Base::Color>(*value)) {
                return colorPreview(std::get<Base::Color>(*value).asValue<QColor>());
            }
        }
    }

    if (auto groupItem = item<GroupItem>(index)) {
        if (role == Qt::DisplayRole && index.column() == ParameterName) {
            return groupItem->title;
        }
    }

    return {};
}

bool StyleParametersModel::setData(
    const QModelIndex& index,
    const QVariant& value,
    [[maybe_unused]] int role
)
{
    if (auto parameterItem = item<ParameterItem>(index)) {
        auto groupItem = item<GroupItem>(index.parent());

        if (index.column() == ParameterName) {
            QString newName = value.toString();

            StyleParameters::Parameter newToken = parameterItem->token;
            newToken.name = newName.toStdString();

            // there is no rename operation, so we need to mark the previous token as deleted
            groupItem->deleted.insert(parameterItem->token.name);

            parameterItem->name = newName;
            parameterItem->token = newToken;
        }

        if (index.column() == ParameterExpression) {
            QString newValue = value.toString();

            StyleParameters::Parameter newToken = parameterItem->token;
            newToken.value = newValue.toStdString();

            parameterItem->token = newToken;
        }
    }

    if (isAddPlaceholder(index)) {
        if (index.column() == ParameterName) {
            QString newName = value.toString();

            if (newName.isEmpty()) {
                return false;
            }

            StyleParameters::Parameter token {.name = newName.toStdString(), .value = ""};

            int start = rowCount(index.parent());

            beginInsertRows(index.parent(), start, start + 1);
            auto item = std::make_unique<Node>(std::make_unique<ParameterItem>(newName, token));
            node(index.parent())->appendChild(std::move(item));
            endInsertRows();

            // this must be queued to basically next frame so widget has a chance to update
            QTimer::singleShot(0, [this, index]() { this->newParameterAdded(index); });
        }
    }

    this->manager->reload();

    QtTools::walkTreeModel(this, [this](const QModelIndex& index) {
        const QModelIndex previewColumnIndex = index.siblingAtColumn(ParameterPreview);

        Q_EMIT dataChanged(previewColumnIndex, previewColumnIndex);
    });

    return true;
}

Qt::ItemFlags StyleParametersModel::flags(const QModelIndex& index) const
{
    if (auto parameterItem = item<ParameterItem>(index)) {
        if (index.column() == ParameterName || index.column() == ParameterExpression) {
            return parameterItem->flags | QAbstractItemModel::flags(index);
        }
    }

    if (isAddPlaceholder(index)) {
        if (index.column() == ParameterName) {
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | QAbstractItemModel::flags(index);
        }
    }

    return QAbstractItemModel::flags(index);
}

QModelIndex StyleParametersModel::index(int row, int col, const QModelIndex& parent) const
{
    if (!hasIndex(row, col, parent)) {
        return {};
    }

    if (auto child = node(parent)->child(row)) {
        return createIndex(row, col, child);
    }

    return {};
}

QModelIndex StyleParametersModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto node = static_cast<Node*>(index.internalPointer());
    auto parent = node->parent();

    if (!parent || parent == root.get()) {
        return {};
    }

    return createIndex(parent->row(), 0, parent);
}

bool StyleParametersModel::isAddPlaceholder(const QModelIndex& index) const
{
    return item(index) == nullptr;
}

StyleParametersModel::Node* StyleParametersModel::node(const QModelIndex& index) const
{
    return index.isValid() ? static_cast<Node*>(index.internalPointer()) : root.get();
}

StyleParametersModel::Item* StyleParametersModel::item(const QModelIndex& index) const
{
    return node(index)->data();
}

DlgThemeEditor::DlgThemeEditor(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DlgThemeEditor)
    , model(
          std::make_unique<StyleParametersModel>(
              Base::provideServiceImplementations<StyleParameters::ParameterSource>(),
              this
          )
      )
{
    ui->setupUi(this);

    ui->tokensTreeView->setMouseTracking(true);
    ui->tokensTreeView->setItemDelegate(new Delegate(ui->tokensTreeView));
    ui->tokensTreeView->setModel(model.get());

    constexpr int typeColumnWidth = 80;
    constexpr int nameColumnWidth = 200;

    struct ColumnDefinition  // NOLINT(*-pro-type-member-init)
    {
        StyleParametersModel::Column column;
        QHeaderView::ResizeMode mode;
        qsizetype defaultWidth = 0;
    };

    static constexpr std::initializer_list<ColumnDefinition> columnSizingDefinitions = {
        {StyleParametersModel::ParameterName, QHeaderView::ResizeMode::ResizeToContents},
        {StyleParametersModel::ParameterExpression, QHeaderView::ResizeMode::Stretch},
        {StyleParametersModel::ParameterPreview, QHeaderView::ResizeMode::Stretch},
        {StyleParametersModel::ParameterType, QHeaderView::ResizeMode::Fixed, typeColumnWidth},
    };

    for (const auto& [column, mode, defaultWidth] : columnSizingDefinitions) {
        ui->tokensTreeView->header()->setSectionResizeMode(column, mode);

        if (defaultWidth > 0) {
            ui->tokensTreeView->header()->setDefaultSectionSize(defaultWidth);
        }
    }

    ui->tokensTreeView->setColumnWidth(StyleParametersModel::ParameterName, nameColumnWidth);
    ui->tokensTreeView->expandAll();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &DlgThemeEditor::handleButtonClick);

    connect(
        ui->tokensTreeView,
        &TokenTreeView::requestRemove,
        model.get(),
        qOverload<const QModelIndex&>(&StyleParametersModel::removeItem)
    );

    connect(model.get(), &StyleParametersModel::modelReset, ui->tokensTreeView, [this] {
        ui->tokensTreeView->expandAll();
    });
    connect(model.get(), &StyleParametersModel::newParameterAdded, this, [this](const QModelIndex& index) {
        const auto newParameterExpressionIndex = index.siblingAtColumn(
            StyleParametersModel::ParameterExpression
        );

        ui->tokensTreeView->scrollTo(newParameterExpressionIndex);
        ui->tokensTreeView->setCurrentIndex(newParameterExpressionIndex);
        ui->tokensTreeView->edit(newParameterExpressionIndex);
    });
}

DlgThemeEditor::~DlgThemeEditor() = default;

void DlgThemeEditor::handleButtonClick(QAbstractButton* button)
{
    auto role = ui->buttonBox->buttonRole(button);

    switch (role) {
        case QDialogButtonBox::ApplyRole:
        case QDialogButtonBox::AcceptRole:
            model->flush();
            Application::Instance->reloadStyleSheet();
            break;
        case QDialogButtonBox::ResetRole:
            model->reset();
            break;
        default:
            // no-op
            break;
    }
}

}  // namespace Gui

#include "DlgThemeEditor.moc"
