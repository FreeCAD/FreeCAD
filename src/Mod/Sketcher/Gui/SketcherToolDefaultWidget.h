// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Pierre-Louis Boyer <pierrelouis.boyer@gmail.com>   *
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


#pragma once

#include <Base/Unit.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Selection/Selection.h>
#include <fastsignals/signal.h>


class QComboBox;

namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
class PrefQuantitySpinBox;
class PrefCheckBox;
class View3DInventorViewer;

class PrefComboBox;
}  // namespace Gui

namespace SketcherGui
{

class Ui_SketcherToolDefaultWidget;
class ViewProviderSketch;

class SketcherToolDefaultWidget: public QWidget
{
    Q_OBJECT

    enum class FontStyle
    {
        Normal,
        Bold,
        Italic,
    };

public:
    /// Parameter spinbox number/label
    enum Parameter
    {
        First,
        Second,
        Third,
        Fourth,
        Fifth,
        Sixth,
        Seventh,
        Eighth,
        Ninth,
        Tenth,
        nParameters  // Must Always be the last one
    };

    /// Checkbox number/label
    enum Checkbox
    {
        FirstBox,
        SecondBox,
        ThirdBox,
        FourthBox,
        nCheckbox  // Must Always be the last one
    };

    /// Combobox number/label
    enum Combobox
    {
        FirstCombo,
        SecondCombo,
        ThirdCombo,
        nCombobox  // Must Always be the last one
    };

    explicit SketcherToolDefaultWidget(QWidget* parent = nullptr);
    ~SketcherToolDefaultWidget() override;

    bool eventFilter(QObject* object, QEvent* event) override;
    // void keyPressEvent(QKeyEvent* event);

    void setParameter(int parameterindex, double val);
    void setParameterWithoutPassingFocus(int parameterindex, double val);
    void configureParameterInitialValue(int parameterindex, double value);
    void configureParameterUnit(int parameterindex, const Base::Unit& unit);
    void configureParameterDecimals(int parameterindex, int val);
    void configureParameterMax(int parameterindex, double val);
    void configureParameterMin(int parameterindex, double val);
    double getParameter(int parameterindex);
    bool isParameterSet(int parameterindex);
    void updateVisualValue(int parameterindex, double val, const Base::Unit& unit = Base::Unit::Length);

    void setParameterEnabled(int parameterindex, bool active = true);
    void setParameterFocus(int parameterindex);

    void setParameterVisible(int parameterindex, bool visible = true);

    void reset();

    void initNParameters(int nparameters, QObject* filteringObject = nullptr);

    void setParameterLabel(int parameterindex, const QString& string);
    void setParameterFilteringObject(int parameterindex, QObject* filteringObject);

    void setNoticeText(const QString& string);
    void setNoticeVisible(bool visible);

    void initNCheckboxes(int ncheckbox);
    void setCheckboxVisible(int checkboxindex, bool visible);
    void setCheckboxChecked(int checkboxindex, bool checked);
    void setCheckboxLabel(int checkboxindex, const QString& string);
    void setCheckboxToolTip(int checkboxindex, const QString& string);
    bool getCheckboxChecked(int checkboxindex);
    void setCheckboxPrefEntry(int checkboxindex, const std::string& prefEntry);
    void setCheckboxIcon(int checkboxindex, QIcon icon);
    void restoreCheckBoxPref(int checkboxindex);

    void initNComboboxes(int ncombobox);
    void setComboboxVisible(int comboboxindex, bool visible);
    void setComboboxIndex(int comboboxindex, int value);
    void setComboboxLabel(int comboboxindex, const QString& string);
    int getComboboxIndex(int comboboxindex);
    void setComboboxElements(int comboboxindex, const QStringList& names);
    void setComboboxItemIcon(int comboboxindex, int index, QIcon icon);
    void setComboboxPrefEntry(int comboboxindex, const std::string& prefEntry);
    void restoreComboboxPref(int comboboxindex);

    template<typename F>
    fastsignals::advanced_connection registerParameterTabOrEnterPressed(F&& fn)
    {
        return signalParameterTabOrEnterPressed.connect(
            std::forward<F>(fn),
            fastsignals::advanced_tag()
        );
    }

    template<typename F>
    fastsignals::advanced_connection registerParameterValueChanged(F&& fn)
    {
        return signalParameterValueChanged.connect(std::forward<F>(fn), fastsignals::advanced_tag());
    }

    template<typename F>
    fastsignals::advanced_connection registerCheckboxCheckedChanged(F&& fn)
    {
        return signalCheckboxCheckedChanged.connect(std::forward<F>(fn), fastsignals::advanced_tag());
    }

    template<typename F>
    fastsignals::advanced_connection registerComboboxSelectionChanged(F&& fn)
    {
        return signalComboboxSelectionChanged.connect(std::forward<F>(fn), fastsignals::advanced_tag());
    }


    // Q_SIGNALS:
protected Q_SLOTS:
    void parameterOne_valueChanged(double val);
    void parameterTwo_valueChanged(double val);
    void parameterThree_valueChanged(double val);
    void parameterFour_valueChanged(double val);
    void parameterFive_valueChanged(double val);
    void parameterSix_valueChanged(double val);
    void parameterSeven_valueChanged(double val);
    void parameterEight_valueChanged(double val);
    void parameterNine_valueChanged(double val);
    void parameterTen_valueChanged(double val);
    void checkBoxTS1_toggled(bool val);
    void checkBoxTS2_toggled(bool val);
    void checkBoxTS3_toggled(bool val);
    void checkBoxTS4_toggled(bool val);
    void comboBox1_currentIndexChanged(int val);
    void comboBox2_currentIndexChanged(int val);
    void comboBox3_currentIndexChanged(int val);

protected:
    void changeEvent(QEvent* ev) override;

private:
    void setupConnections();
    QLabel* getParameterLabel(int parameterindex);
    Gui::PrefQuantitySpinBox* getParameterSpinBox(int parameterindex);
    Gui::PrefCheckBox* getCheckBox(int checkboxindex);
    Gui::PrefComboBox* getComboBox(int comboboxindex);
    QLabel* getComboBoxLabel(int comboboxindex);

    void setParameterFontStyle(int parameterindex, FontStyle fontStyle);

    bool isCheckBoxPrefEntryEmpty(int checkboxindex);

private:
    std::unique_ptr<Ui_SketcherToolDefaultWidget> ui;

    fastsignals::signal<void(int parameterindex)> signalParameterTabOrEnterPressed;
    fastsignals::signal<void(int parameterindex, double value)> signalParameterValueChanged;
    fastsignals::signal<void(int checkboxindex, bool value)> signalCheckboxCheckedChanged;
    fastsignals::signal<void(int comboindex, int value)> signalComboboxSelectionChanged;

    /// lock to block QT slots
    bool blockParameterSlots;
    bool blockParameterFocusPassing;

    /// vector using parameter as index indicating whether the value of a parameter was set by the
    /// widget
    std::vector<bool> isSet;
};


}  // namespace SketcherGui
