/***************************************************************************
 *   Copyright (c) 2022 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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


#ifndef GUI_DIALOG_DLGPREFERENCEPACKMANAGEMENTIMP_H
#define GUI_DIALOG_DLGPREFERENCEPACKMANAGEMENTIMP_H

#include <memory>
#include <QDialog>
#include <boost/filesystem.hpp>
#include <FCGlobal.h>


class QTreeWidgetItem;

namespace Gui {

namespace Dialog {

class Ui_DlgPreferencePackManagement;

/**
 * \class DlgCreateNewPreferencePackImp
 *
 * A dialog to request a preferencePack name and a set of preferencePack templates.
 *
 * \author Chris Hennes
 */
class GuiExport DlgPreferencePackManagementImp : public QDialog
{
  Q_OBJECT

public:

    DlgPreferencePackManagementImp(QWidget* parent = nullptr);
    ~DlgPreferencePackManagementImp() override;

Q_SIGNALS:
    void packVisibilityChanged();

protected Q_SLOTS:

    void deleteUserPack(const std::string & prefPackName);
    void hideBuiltInPack(const std::string& prefPackName);
    void hideInstalledPack(const std::string& addonName, const std::string& prefPackName);

    void showEvent(QShowEvent* event) override;
    void showAddonManager();

private:

    enum class TreeWidgetType {
        BUILTIN,
        USER,
        ADDON
    };

    std::unique_ptr<Ui_DlgPreferencePackManagement> ui;

    std::vector<std::string> getPacksFromDirectory(const boost::filesystem::path& path) const;
    void addTreeNode(const std::string& name, const std::vector<std::string>& contents, TreeWidgetType twt);

};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGPREFERENCEPACKMANAGEMENTIMP_H
