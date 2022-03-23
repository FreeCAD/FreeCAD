/***************************************************************************
 *   Copyright (c) 2020 Chris Hennes (chennes@pioneerlibrarysystem.org)    *
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


#ifndef GUI_DIALOG_DLGSETTINGSLAZYLOADED_IMP_H
#define GUI_DIALOG_DLGSETTINGSLAZYLOADED_IMP_H

#include "PropertyPage.h"
#include <memory>

class QCheckBox;

namespace Gui {
namespace Dialog {
class Ui_DlgSettingsLazyLoaded;


/**
 * The DlgSettingsLazyLoadedImp class implements a pseudo-preference page explain why
 * the remaining preference pages aren't loaded yet, and to help the user do so on demand.
 * \author Jürgen Riegel
 */
class DlgSettingsLazyLoadedImp : public PreferencePage
{
    Q_OBJECT

public:
    DlgSettingsLazyLoadedImp( QWidget* parent = nullptr );
    ~DlgSettingsLazyLoadedImp();

    void saveSettings();
    void loadSettings();

protected Q_SLOTS:
    void onLoadClicked(const QString& wbName);

protected:
    void buildUnloadedWorkbenchList();
    void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui_DlgSettingsLazyLoaded> ui;
    static const uint WorkbenchNameRole;

    std::vector<std::string> _backgroundAutoloadedModules;
    std::string _startupModule;
    std::map<QString, QCheckBox*> _autoloadCheckboxes;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGSLAZYLOADED_IMP_H
