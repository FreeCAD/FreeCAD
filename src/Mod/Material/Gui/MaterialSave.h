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

#ifndef MATGUI_MATERIALSAVE_H
#define MATGUI_MATERIALSAVE_H

// #include <boost/filesystem.hpp>

#include <QDialog>
// #include <QDir>
// #include <QStandardItem>
// #include <QTreeView>
// #include <QStyledItemDelegate>
// #include <QSvgWidget>

// #include <Mod/Material/App/Materials.h>
// #include <Mod/Material/App/MaterialManager.h>
// #include <Mod/Material/App/ModelManager.h>

// namespace fs = boost::filesystem;

namespace MatGui {

class Ui_MaterialSave;

class MaterialSave : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialSave(QWidget* parent = nullptr);
    ~MaterialSave() override;

    void accept() override;
    void reject() override;

private:
    std::unique_ptr<Ui_MaterialSave> ui;
};

} // namespace MatGui

#endif // MATGUI_MATERIALSAVE_H
