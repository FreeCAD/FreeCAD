/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef SKETCHERGUI_SKETCHERSETTINGS_H
#define SKETCHERGUI_SKETCHERSETTINGS_H

#include <Gui/PropertyPage.h>

namespace SketcherGui {
class Ui_SketcherSettings;
class Ui_SketcherSettingsColors;
class SketcherGeneralWidget;
/**
 * The SketcherSettings class implements a preference page to change sketcher settings.
 * @author Werner Mayer
 */
class SketcherSettings : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    SketcherSettings(QWidget* parent = 0);
    ~SketcherSettings();

    void saveSettings();
    void loadSettings();

protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void onBtnTVApplyClicked(bool);

private:
    Ui_SketcherSettings* ui;
    SketcherGeneralWidget* form;
};

/**
 * The SketcherSettings class implements a preference page to change sketcher settings.
 * @author Werner Mayer
 */
class SketcherSettingsColors : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    SketcherSettingsColors(QWidget* parent = 0);
    ~SketcherSettingsColors();

    void saveSettings();
    void loadSettings();

protected:
    void changeEvent(QEvent *e);

private:
    Ui_SketcherSettingsColors* ui;
};

} // namespace SketcherGui

#endif // SKETCHERGUI_SKETCHERSETTINGS_H
