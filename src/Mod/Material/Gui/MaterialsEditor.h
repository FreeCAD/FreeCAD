/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef MATGUI_MATERIALSEDITOR_H
#define MATGUI_MATERIALSEDITOR_H

#include <boost/filesystem.hpp>

#include <QDialog>
#include <QDir>
#include <QStandardItem>
#include <memory>

class QTreeWidgetItem;
namespace fs = boost::filesystem;

namespace MatGui {

// using Connection = boost::signals2::connection;
class Ui_MaterialsEditor;
class LibraryData;
class MaterialsEditor : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialsEditor(QWidget* parent = nullptr);
    ~MaterialsEditor() override;
    void accept() override;
    void reject() override;

private:
    std::unique_ptr<Ui_MaterialsEditor> ui;

    void tryPython();
    void createMaterialTree();
    void addCards(QStandardItem &parent, const std::string &top, const std::string &folder, const QIcon &icon);
    bool isCard(const fs::path &p) const;
    std::list<LibraryData *> *getMaterialLibraries();
};

class LibraryData
{
public:
    explicit LibraryData(const std::string &libraryName, QDir *dir, const std::string &icon);
    virtual ~LibraryData();

    const std::string &getName() const
    {
        return name;
    }
    QDir& getDirectory() const
    {
        return *directory;
    }
    const std::string &getIconPath() const
    {
        return iconPath;
    }

private:
    explicit LibraryData();
    std::string name;
    QDir* directory;
    std::string iconPath;
};

} // namespace MatGui

#endif // MATGUI_MATERIALSEDITOR_H
