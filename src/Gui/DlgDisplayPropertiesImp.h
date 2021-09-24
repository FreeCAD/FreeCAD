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


#ifndef GUI_DIALOG_DLGDISPLAYPROPERTIES_IMP_H
#define GUI_DIALOG_DLGDISPLAYPROPERTIES_IMP_H

#include <memory>
#include <vector>
#include <QDialog>

#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <App/Material.h>

namespace App
{
  class Property;
}

namespace Gui {

  class ViewProvider;
  class Command;

namespace Dialog {

/**
 * The DlgDisplayPropertiesImp class implements a dialog containing all available document
 * templates to create a new document.
 * \author Jürgen Riegel
 */
class DlgDisplayPropertiesImp : public QDialog,
                                public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    DlgDisplayPropertiesImp(bool floating, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgDisplayPropertiesImp();
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason);
    void showDefaultButtons(bool);
    void reject();

private Q_SLOTS:
    void on_changeMaterial_activated(int);
    void on_changeMode_activated(const QString&);
    void on_changePlot_activated(const QString&);
    void on_buttonColor_changed();
    void on_spinTransparency_valueChanged(int);
    void on_spinPointSize_valueChanged(int);
    void on_buttonLineColor_changed();
    void on_spinLineWidth_valueChanged(int);
    void on_spinLineTransparency_valueChanged(int);
    void on_buttonUserDefinedMaterial_clicked();
    void on_buttonColorPlot_clicked();

protected:
    void changeEvent(QEvent *e);

private:
    void slotChangedObject(const Gui::ViewProvider&, const App::Property& Prop);
    void setDisplayModes(const std::vector<ViewProvider*>&);
    void setMaterial(const std::vector<ViewProvider*>&);
    void setColorPlot(const std::vector<ViewProvider*>&);
    void fillupMaterials();
    void setShapeColor(const std::vector<ViewProvider*>&);
    void setLineColor(const std::vector<ViewProvider*>&);
    void setPointSize(const std::vector<ViewProvider*>&);
    void setLineWidth(const std::vector<ViewProvider*>&);
    void setTransparency(const std::vector<ViewProvider*>&);
    void setLineTransparency(const std::vector<ViewProvider*>&);
    std::vector<ViewProvider*> getSelection() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

class TaskDisplayProperties : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDisplayProperties();
    ~TaskDisplayProperties();

public:
    bool reject();

    bool isAllowedAlterDocument(void) const
    { return true; }
    bool isAllowedAlterView(void) const
    { return true; }
    bool isAllowedAlterSelection(void) const
    { return true; }
    QDialogButtonBox::StandardButtons getStandardButtons() const;

private:
    DlgDisplayPropertiesImp* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGDISPLAYPROPERTIES_IMP_H
