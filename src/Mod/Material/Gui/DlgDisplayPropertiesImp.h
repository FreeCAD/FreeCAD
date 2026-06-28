// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <QDialog>
#include <memory>
#include <vector>

#include <App/Material.h>
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include <Mod/Material/App/Materials.h>

namespace App
{
class Property;
}

namespace MatGui
{

class ViewProvider;
class Command;

/**
 * The DlgDisplayPropertiesImp class implements a dialog containing all available document
 * templates to create a new document.
 * \author Jürgen Riegel
 */
class DlgDisplayPropertiesImp: public QDialog, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    explicit DlgDisplayPropertiesImp(QWidget* parent = nullptr,
                                     Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgDisplayPropertiesImp() override;
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                  Gui::SelectionSingleton::MessageType Reason) override;
    void showDefaultButtons(bool);
    void reject() override;

private Q_SLOTS:
    void onChangeModeActivated(const QString&);
    void onChangePlotActivated(const QString&);
    void onSpinTransparencyValueChanged(int);
    void onSpinPointSizeValueChanged(int);
    void onButtonLineColorChanged();
    void onButtonPointColorChanged();
    void onSpinLineWidthValueChanged(int);
    void onSpinLineTransparencyValueChanged(int);
    void onButtonCustomAppearanceClicked();
    void onButtonColorPlotClicked();
    void onMaterialSelected(const std::shared_ptr<Materials::Material>& material);

protected:
    void changeEvent(QEvent* e) override;

private:
    void setupConnections();
    void setupFilters();
    void slotChangedObject(const Gui::ViewProvider&, const App::Property& Prop);
    void setDisplayModes(const std::vector<Gui::ViewProvider*>&);
    void setColorPlot(const std::vector<Gui::ViewProvider*>&);
    void setShapeAppearance(const std::vector<Gui::ViewProvider*>&);
    void setLineColor(const std::vector<Gui::ViewProvider*>&);
    void setPointColor(const std::vector<Gui::ViewProvider*>&);
    void setPointSize(const std::vector<Gui::ViewProvider*>&);
    void setLineWidth(const std::vector<Gui::ViewProvider*>&);
    void setTransparency(const std::vector<Gui::ViewProvider*>&);
    void setLineTransparency(const std::vector<Gui::ViewProvider*>&);
    std::vector<Gui::ViewProvider*> getSelection() const;
    void setPropertiesFromSelection();

private:
    class Private;
    std::unique_ptr<Private> d;
};

class TaskDisplayProperties: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDisplayProperties();
    ~TaskDisplayProperties() override;

public:
    bool reject() override;

    bool isAllowedAlterDocument() const override
    {
        return true;
    }
    bool isAllowedAlterView() const override
    {
        return true;
    }
    bool isAllowedAlterSelection() const override
    {
        return true;
    }
    QDialogButtonBox::StandardButtons getStandardButtons() const override;

private:
    DlgDisplayPropertiesImp* widget;
};

}  // namespace MatGui