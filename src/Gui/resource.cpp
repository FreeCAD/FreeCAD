/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "BitmapFactory.h"
#include "WidgetFactory.h"
#include "Workbench.h"

// INCLUDE YOUR PREFERENCE PAGES HERE
//
#include "DlgPreferencesImp.h"
#include "PreferencePages/DlgSettings3DViewImp.h"
#include "PreferencePages/DlgSettingsCacheDirectory.h"
#include "PreferencePages/DlgSettingsDocumentImp.h"
#include "PreferencePages/DlgSettingsEditor.h"
#include "PreferencePages/DlgSettingsGeneral.h"
#include "PreferencePages/DlgSettingsMacroImp.h"
#include "PreferencePages/DlgSettingsNavigation.h"
#include "PreferencePages/DlgSettingsNotificationArea.h"
#include "PreferencePages/DlgSettingsPythonConsole.h"
#include "PreferencePages/DlgSettingsReportView.h"
#include "PreferencePages/DlgSettingsSelection.h"
#include "PreferencePages/DlgSettingsTheme.h"
#include "PreferencePages/DlgSettingsViewColor.h"
#include "PreferencePages/DlgSettingsWorkbenchesImp.h"


#include "DlgToolbarsImp.h"
#include "DlgActionsImp.h"
#include "DlgKeyboardImp.h"
#include "DlgCustomizeSpaceball.h"
#include "DlgCustomizeSpNavSettings.h"
#include "InputField.h"
#include "QuantitySpinBox.h"
#include "PrefWidgets.h"

using namespace Gui;
using namespace Gui::Dialog;

/**
 * Registers all preference pages or widgets to create them dynamically at any later time.
 */
WidgetFactorySupplier::WidgetFactorySupplier()
{
    // ADD YOUR PREFERENCE PAGES HERE
    //
    //
    new PrefPageProducer<DlgSettingsGeneral>          ( QT_TRANSLATE_NOOP("QObject","General") );
    new PrefPageProducer<DlgSettingsDocumentImp>      ( QT_TRANSLATE_NOOP("QObject","General") );
    new PrefPageProducer<DlgSettingsSelection>        ( QT_TRANSLATE_NOOP("QObject","General") );
    new PrefPageProducer<DlgSettingsCacheDirectory>   ( QT_TRANSLATE_NOOP("QObject","General") );
    new PrefPageProducer<DlgSettingsNotificationArea> ( QT_TRANSLATE_NOOP("QObject","General") );
    new PrefPageProducer<DlgSettingsReportView>       ( QT_TRANSLATE_NOOP("QObject","General") );
    new PrefPageProducer<DlgSettings3DViewImp>        ( QT_TRANSLATE_NOOP("QObject","Display") );
    new PrefPageProducer<DlgSettingsNavigation>       ( QT_TRANSLATE_NOOP("QObject","Display") );
    new PrefPageProducer<DlgSettingsViewColor>        ( QT_TRANSLATE_NOOP("QObject","Display") );
    new PrefPageProducer<DlgSettingsTheme>            ( QT_TRANSLATE_NOOP("QObject","Display") );
    new PrefPageProducer<DlgSettingsWorkbenchesImp>   ( QT_TRANSLATE_NOOP("QObject","Workbenches") );
    new PrefPageProducer<DlgSettingsMacroImp>         ( QT_TRANSLATE_NOOP("QObject", "Python"));
    new PrefPageProducer<DlgSettingsPythonConsole>    ( QT_TRANSLATE_NOOP("QObject", "Python"));
    new PrefPageProducer<DlgSettingsEditor>           ( QT_TRANSLATE_NOOP("QObject", "Python"));

    // ADD YOUR CUSTOMIZE PAGES HERE
    //
    //
    new CustomPageProducer<DlgCustomKeyboardImp>;
    new CustomPageProducer<DlgCustomToolbarsImp>;
    new CustomPageProducer<DlgCustomActionsImp>;
    new CustomPageProducer<DlgCustomizeSpNavSettings>;
    new CustomPageProducer<DlgCustomizeSpaceball>;

    // ADD YOUR PREFERENCE WIDGETS HERE
    //
    //
    new WidgetProducer<Gui::InputField>;
    new WidgetProducer<Gui::PrefSpinBox>;
    new WidgetProducer<Gui::PrefDoubleSpinBox>;
    new WidgetProducer<Gui::PrefLineEdit>;
    new WidgetProducer<Gui::PrefTextEdit>;
    new WidgetProducer<Gui::PrefComboBox>;
    new WidgetProducer<Gui::PrefFontBox>;
    new WidgetProducer<Gui::PrefCheckBox>;
    new WidgetProducer<Gui::PrefRadioButton>;
    new WidgetProducer<Gui::PrefSlider>;
    new WidgetProducer<Gui::PrefFileChooser>;
    new WidgetProducer<Gui::PrefColorButton>;
    new WidgetProducer<Gui::PrefUnitSpinBox>;
    new WidgetProducer<Gui::PrefQuantitySpinBox>;
    new WidgetProducer<Gui::CommandIconView>;
    new WidgetProducer<Gui::AccelLineEdit>;
    new WidgetProducer<Gui::ActionSelector>;
    new WidgetProducer<Gui::ColorButton>;
    new WidgetProducer<Gui::UrlLabel>;
    new WidgetProducer<Gui::StatefulLabel>;
    new WidgetProducer<Gui::FileChooser>;
    new WidgetProducer<Gui::UIntSpinBox>;
    new WidgetProducer<Gui::IntSpinBox>;
    new WidgetProducer<Gui::DoubleSpinBox>;
    new WidgetProducer<Gui::QuantitySpinBox>;
}
