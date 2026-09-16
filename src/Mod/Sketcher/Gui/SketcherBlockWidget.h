// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QWidget>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace SketcherGui
{
class SketcherBlockWidget: public QWidget
{
    Q_OBJECT
public:
    explicit SketcherBlockWidget(QWidget* parent = nullptr);
    QString selectedFile() const;
    void setPlacementOptions(int method, bool fixedSize, bool fixedOrientation, bool customHandle = false);
    void toggleFixedSize();
    void toggleFixedOrientation();

Q_SIGNALS:
    void fileSelected(const QString& path);
    void placementChanged(int method, bool fixedSize, bool fixedOrientation);

private:
    QTreeWidget* tree;
    QComboBox* method;
    QCheckBox* fixedSize;
    QCheckBox* fixedOrientation;
    QPushButton* removeFolder;
    QLabel* selectedLabel;
    QString filename;
    QStringList customFolders;
    void refresh();
    void populate(QTreeWidgetItem* item);
    void addRoot(const QString& path, const QString& label, bool removable);
    bool selectFile(const QString& path);
    bool selectFirstFile(QTreeWidgetItem* item);
    void setFile(const QString& path);
    QString selectedDirectory() const;
    void saveFolders();
    void chooseFolder();
    void removeSelectedFolder();
    void chooseFile();
};
}  // namespace SketcherGui
