/***************************************************************************
 *   Copyright (c) 2021 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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


#ifndef GUI_DIALOG_DLGCREATENEWTHEMEIMP_H
#define GUI_DIALOG_DLGCREATENEWTHEMEIMP_H

#include <memory>
#include <QDialog>
#include <QRegularExpressionValidator>

#include "PreferencePackManager.h"

class QTreeWidgetItem;

namespace Gui {

namespace Dialog {

class Ui_DlgCreateNewPreferencePack;

/**
 * \class DlgCreateNewPreferencePackImp
 *
 * A dialog to request a preferencePack name and a set of preferencePack templates.
 *
 * \author Chris Hennes
 */
class GuiExport DlgCreateNewPreferencePackImp : public QDialog
{
  Q_OBJECT

public:

    explicit DlgCreateNewPreferencePackImp(QWidget* parent = nullptr);
    ~DlgCreateNewPreferencePackImp() override;

    void setPreferencePackTemplates(const std::vector<PreferencePackManager::TemplateFile> &availableTemplates);
    void setPreferencePackNames(const std::vector<std::string>& usedNames);

    std::vector<PreferencePackManager::TemplateFile> selectedTemplates() const;
    std::string preferencePackName() const;

protected Q_SLOTS:

    void onItemChanged(QTreeWidgetItem* item, int column);

    void onLineEditTextEdited(const QString &text);

    void accept() override;

private:
    std::unique_ptr<Ui_DlgCreateNewPreferencePack> ui;
    std::map<std::string, QTreeWidgetItem*> _groups;
    std::vector<PreferencePackManager::TemplateFile> _templates;
    QRegularExpressionValidator _nameValidator;
    std::vector<std::string> _existingPackNames;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGCREATENEWTHEMEIMP_H
