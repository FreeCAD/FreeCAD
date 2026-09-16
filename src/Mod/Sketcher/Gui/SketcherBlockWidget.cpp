// SPDX-License-Identifier: LGPL-2.1-or-later
#include "PreCompiled.h"
#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <App/Application.h>
#include <Gui/FileDialog.h>
#include "SketcherBlockWidget.h"

using namespace SketcherGui;
namespace
{
constexpr int pathRole = Qt::UserRole;
constexpr int folderRole = Qt::UserRole + 1;
constexpr int populatedRole = Qt::UserRole + 2;
constexpr int removableRole = Qt::UserRole + 3;
QString builtInFolder()
{
    return QString::fromStdString(App::Application::getResourceDir() + "Mod/Sketcher/Blocks/");
}
ParameterGrp::handle preferences()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/BlockLibrary"
    );
}
QString canonicalPath(const QString& path)
{
    const QFileInfo info(path);
    return QDir::cleanPath(info.exists() ? info.canonicalFilePath() : info.absoluteFilePath());
}
bool samePath(const QString& first, const QString& second)
{
#ifdef Q_OS_WIN
    return canonicalPath(first).compare(canonicalPath(second), Qt::CaseInsensitive) == 0;
#else
    return canonicalPath(first) == canonicalPath(second);
#endif
}
}  // namespace

SketcherBlockWidget::SketcherBlockWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("SketcherBlockWidget"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    tree = new QTreeWidget(this);
    tree->setObjectName(QStringLiteral("blockLibraryTree"));
    tree->setHeaderLabel(tr("Block Library"));
    tree->setMinimumHeight(220);
    tree->setUniformRowHeights(true);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(tree, 1);
    auto* folders = new QHBoxLayout;
    auto* add = new QPushButton(tr("Add Folder…"), this);
    add->setObjectName(QStringLiteral("addBlockFolder"));
    removeFolder = new QPushButton(tr("Remove Folder"), this);
    removeFolder->setObjectName(QStringLiteral("removeBlockFolder"));
    removeFolder->setToolTip(tr("Removes the folder from this library list; files are kept"));
    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    refreshButton->setObjectName(QStringLiteral("refreshBlockLibrary"));
    folders->addWidget(add);
    folders->addWidget(removeFolder);
    folders->addWidget(refreshButton);
    layout->addLayout(folders);
    auto* choose = new QPushButton(tr("Choose File…"), this);
    choose->setObjectName(QStringLiteral("chooseBlockFile"));
    layout->addWidget(choose);
    selectedLabel = new QLabel(this);
    selectedLabel->setTextFormat(Qt::PlainText);
    selectedLabel->setWordWrap(true);
    layout->addWidget(selectedLabel);
    fixedSize = new QCheckBox(tr("Fixed Size"), this);
    fixedSize->setObjectName(QStringLiteral("blockFixedSize"));
    fixedSize->setChecked(false);
    fixedSize->setToolTip(tr("Preserves the size from the block file"));
    fixedOrientation = new QCheckBox(tr("Fixed Orientation"), this);
    fixedOrientation->setObjectName(QStringLiteral("blockFixedOrientation"));
    fixedOrientation->setToolTip(tr("Preserves the orientation from the block file"));
    layout->addWidget(fixedSize);
    layout->addWidget(fixedOrientation);
    method = new QComboBox(this);
    method->setObjectName(QStringLiteral("blockSizeMode"));
    method->addItems({tr("Width"), tr("Height")});
    auto* form = new QFormLayout;
    form->addRow(tr("Size Mode"), method);
    layout->addLayout(form);
    auto emitPlacement = [this]() {
        Q_EMIT placementChanged(
            method->isEnabled() ? method->currentIndex() : 0,
            fixedSize->isChecked(),
            fixedOrientation->isChecked()
        );
    };
    connect(method, qOverload<int>(&QComboBox::currentIndexChanged), this, emitPlacement);
    connect(fixedSize, &QCheckBox::toggled, this, emitPlacement);
    connect(fixedOrientation, &QCheckBox::toggled, this, emitPlacement);
    connect(add, &QPushButton::clicked, this, &SketcherBlockWidget::chooseFolder);
    connect(removeFolder, &QPushButton::clicked, this, &SketcherBlockWidget::removeSelectedFolder);
    connect(refreshButton, &QPushButton::clicked, this, &SketcherBlockWidget::refresh);
    connect(choose, &QPushButton::clicked, this, &SketcherBlockWidget::chooseFile);
    connect(tree, &QTreeWidget::itemExpanded, this, &SketcherBlockWidget::populate);
    connect(tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
        auto* root = item;
        while (root && root->parent()) {
            root = root->parent();
        }
        removeFolder->setEnabled(root && root->data(0, removableRole).toBool());
        setFile(
            item && !item->data(0, folderRole).toBool() ? item->data(0, pathRole).toString()
                                                        : QString()
        );
    });
    const auto stored = QByteArray::fromStdString(preferences()->GetASCII("Folders", "[]"));
    for (const auto value : QJsonDocument::fromJson(stored).array()) {
        if (value.isString() && !value.toString().isEmpty()) {
            customFolders.append(value.toString());
        }
    }
    filename = QString::fromStdString(preferences()->GetASCII("LastFile", ""));
    refresh();
}

QString SketcherBlockWidget::selectedFile() const
{
    return filename;
}
void SketcherBlockWidget::setPlacementOptions(int mode, bool size, bool orientation, bool customHandle)
{
    const QSignalBlocker modeBlock(method), sizeBlock(fixedSize), orientationBlock(fixedOrientation);
    if (customHandle && method->count() == 2) {
        method->addItem(tr("Handle Length"));
    }
    else if (!customHandle && method->count() > 2) {
        method->removeItem(2);
    }
    method->setCurrentIndex(customHandle ? 2 : mode);
    method->setEnabled(!customHandle);
    fixedSize->setChecked(size);
    fixedOrientation->setChecked(orientation);
}
void SketcherBlockWidget::toggleFixedSize()
{
    fixedSize->toggle();
}
void SketcherBlockWidget::toggleFixedOrientation()
{
    fixedOrientation->toggle();
}

void SketcherBlockWidget::addRoot(const QString& path, const QString& label, bool removable)
{
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (samePath(tree->topLevelItem(i)->data(0, pathRole).toString(), path)) {
            return;
        }
    }
    auto* item = new QTreeWidgetItem(tree, {label.isEmpty() ? QDir::toNativeSeparators(path) : label});
    item->setData(0, pathRole, canonicalPath(path));
    item->setData(0, folderRole, true);
    item->setData(0, removableRole, removable);
    item->setToolTip(0, QDir::toNativeSeparators(canonicalPath(path)));
    item->setIcon(0, QFileIconProvider().icon(QFileIconProvider::Folder));
    if (QDir(path).exists()) {
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
    else {
        item->setText(0, tr("%1 (Unavailable)").arg(label));
    }
}

void SketcherBlockWidget::populate(QTreeWidgetItem* item)
{
    if (!item->data(0, folderRole).toBool() || item->data(0, populatedRole).toBool()) {
        return;
    }
    item->setData(0, populatedRole, true);
    const QDir directory(item->data(0, pathRole).toString());
    const auto entries = directory.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase
    );
    QFileIconProvider icons;
    for (const auto& entry : entries) {
        if (!entry.isDir()
            && entry.suffix().compare(QStringLiteral("txt"), Qt::CaseInsensitive) != 0) {
            continue;
        }
        auto* child = new QTreeWidgetItem(
            item,
            {entry.isDir() ? entry.fileName() : entry.completeBaseName()}
        );
        child->setData(0, pathRole, entry.absoluteFilePath());
        child->setData(0, folderRole, entry.isDir());
        child->setToolTip(0, QDir::toNativeSeparators(entry.absoluteFilePath()));
        child->setIcon(
            0,
            icons.icon(entry.isDir() ? QFileIconProvider::Folder : QFileIconProvider::File)
        );
        if (entry.isDir()) {
            child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
    }
    item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
}

bool SketcherBlockWidget::selectFile(const QString& path)
{
    if (path.isEmpty() || !QFileInfo(path).isFile()) {
        return false;
    }
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        auto* item = tree->topLevelItem(i);
        const QString relative
            = QDir(item->data(0, pathRole).toString()).relativeFilePath(canonicalPath(path));
        if (relative.startsWith(QStringLiteral("../")) || QDir::isAbsolutePath(relative)) {
            continue;
        }
        const auto parts = relative.split('/');
        for (const auto& part : parts) {
            populate(item);
            item->setExpanded(true);
            QTreeWidgetItem* next = nullptr;
            for (int j = 0; j < item->childCount(); ++j) {
                if (samePath(
                        item->child(j)->data(0, pathRole).toString(),
                        QDir(item->data(0, pathRole).toString()).filePath(part)
                    )) {
                    next = item->child(j);
                    break;
                }
            }
            item = next;
            if (!item) {
                break;
            }
        }
        if (item && !item->data(0, folderRole).toBool()) {
            tree->setCurrentItem(item);
            tree->scrollToItem(item);
            return true;
        }
    }
    return false;
}

bool SketcherBlockWidget::selectFirstFile(QTreeWidgetItem* item)
{
    populate(item);
    for (int i = 0; i < item->childCount(); ++i) {
        auto* child = item->child(i);
        if (!child->data(0, folderRole).toBool()) {
            item->setExpanded(true);
            tree->setCurrentItem(child);
            return true;
        }
    }
    return false;
}

void SketcherBlockWidget::refresh()
{
    const QString previous = filename;
    tree->clear();
    removeFolder->setEnabled(false);
    addRoot(builtInFolder(), tr("Built-in Blocks"), false);
    addRoot(
        QString::fromStdString(App::Application::getUserAppDataDir() + "Mod/Sketcher/Blocks/"),
        tr("User Blocks"),
        false
    );
    for (const auto& folder : customFolders) {
        addRoot(folder, QFileInfo(folder).fileName(), true);
    }
    if (selectFile(previous)) {
        return;
    }
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (selectFirstFile(tree->topLevelItem(i))) {
            return;
        }
    }
    setFile(QString());
}

void SketcherBlockWidget::setFile(const QString& path)
{
    filename = path;
    selectedLabel->setText(path.isEmpty() ? tr("Select a block") : QFileInfo(path).fileName());
    selectedLabel->setToolTip(QDir::toNativeSeparators(path));
    if (!path.isEmpty()) {
        preferences()->SetASCII("LastFile", path.toUtf8().constData());
    }
    Q_EMIT fileSelected(path);
}

QString SketcherBlockWidget::selectedDirectory() const
{
    if (const auto* item = tree->currentItem()) {
        const QString path = item->data(0, pathRole).toString();
        if (QFileInfo(path).exists()) {
            return item->data(0, folderRole).toBool() ? path : QFileInfo(path).absolutePath();
        }
    }
    return builtInFolder();
}

void SketcherBlockWidget::saveFolders()
{
    QJsonArray array;
    for (const auto& folder : customFolders) {
        array.append(folder);
    }
    preferences()->SetASCII("Folders", QJsonDocument(array).toJson(QJsonDocument::Compact).constData());
}

void SketcherBlockWidget::chooseFolder()
{
    const QString chosen
        = Gui::FileDialog::getExistingDirectory(this, tr("Add Block Folder"), selectedDirectory());
    if (chosen.isEmpty()) {
        return;
    }
    const QString path = canonicalPath(chosen);
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (samePath(path, tree->topLevelItem(i)->data(0, pathRole).toString())) {
            tree->setCurrentItem(tree->topLevelItem(i));
            tree->topLevelItem(i)->setExpanded(true);
            return;
        }
    }
    customFolders.append(path);
    saveFolders();
    addRoot(path, QFileInfo(path).fileName(), true);
    auto* item = tree->topLevelItem(tree->topLevelItemCount() - 1);
    tree->setCurrentItem(item);
    item->setExpanded(true);
}

void SketcherBlockWidget::removeSelectedFolder()
{
    auto* item = tree->currentItem();
    while (item && item->parent()) {
        item = item->parent();
    }
    if (!item || !item->data(0, removableRole).toBool()) {
        return;
    }
    const QString path = item->data(0, pathRole).toString();
    customFolders.erase(
        std::remove_if(
            customFolders.begin(),
            customFolders.end(),
            [&path](const QString& folder) { return samePath(path, folder); }
        ),
        customFolders.end()
    );
    saveFolders();
    refresh();
}

void SketcherBlockWidget::chooseFile()
{
    const QString path = Gui::FileDialog::getOpenFileName(
        this,
        tr("Insert Block"),
        selectedDirectory(),
        {{tr("Sketcher block files"), {QStringLiteral("*.txt")}}}
    );
    if (path.isEmpty()) {
        return;
    }
    if (!selectFile(path)) {
        const QSignalBlocker blocker(tree);
        tree->setCurrentItem(nullptr);
        removeFolder->setEnabled(false);
        setFile(path);
    }
}
