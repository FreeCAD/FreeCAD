/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "PreCompiled.h"

#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>

#include "DlgSettingsMacroImp.h"
#include "ui_DlgSettingsMacro.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsMacroImp */

/**
 *  Constructs a DlgSettingsMacroImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsMacroImp::DlgSettingsMacroImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgSettingsMacro)
{
    ui->setupUi(this);

    // Was never implemented, so hide it
    ui->FileLogCheckBox->hide();
    ui->MacroPath_2->hide();

    if (ui->MacroPath->fileName().isEmpty()) {
        QDir d(QString::fromUtf8(App::GetApplication().getUserMacroDir().c_str()));
        ui->MacroPath->setFileName(d.path());
    }
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsMacroImp::~DlgSettingsMacroImp() = default;

/** Sets the size of the recent macros list from the user parameters.
 * @see RecentMacrosAction
 * @see StdCmdRecentMacros
 */
void DlgSettingsMacroImp::setRecentMacroSize()
{
    auto recent = getMainWindow()->findChild<RecentMacrosAction *>(QLatin1String("recentMacros"));
    if (recent) {
        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("RecentMacros");
        recent->resizeList(hGrp->GetInt("RecentMacros", 4));
    }
}

void DlgSettingsMacroImp::saveSettings()
{
    ui->PrefCheckBox_LocalEnv->onSave();
    ui->MacroPath->onSave();
    ui->PrefCheckBox_RecordGui->onSave();
    ui->PrefCheckBox_GuiAsComment->onSave();
    ui->PConsoleCheckBox->onSave();
    ui->FileLogCheckBox->onSave();
    ui->MacroPath_2->onSave();
    ui->RecentMacros->onSave();
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("RecentMacros");
    hGrp->SetASCII("ShortcutModifiers", qPrintable(ui->ShortcutModifiers->text()));
    ui->ShortcutCount->onSave();
    setRecentMacroSize();
}

void DlgSettingsMacroImp::loadSettings()
{
    ui->PrefCheckBox_LocalEnv->onRestore();
    ui->MacroPath->onRestore();
    ui->PrefCheckBox_RecordGui->onRestore();
    ui->PrefCheckBox_GuiAsComment->onRestore();
    ui->PConsoleCheckBox->onRestore();
    ui->FileLogCheckBox->onRestore();
    ui->MacroPath_2->onRestore();
    ui->RecentMacros->onRestore();
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("RecentMacros");
    ui->ShortcutModifiers->setText(QString::fromStdString(hGrp->GetASCII("ShortcutModifiers", "Ctrl+Shift+")));
    ui->ShortcutCount->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsMacroImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsMacroImp.cpp"
