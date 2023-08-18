/***************************************************************************
 *   Copyright (c) 2011 Joe Dowsett <j-dowsett[at]users.sourceforge.net>   *
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef GUI_TASKVIEW_TASKVIEWGROUP_H
#define GUI_TASKVIEW_TASKVIEWGROUP_H

#include <QString>

#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw {
class DrawProjGroup;
class DrawPage;
}

namespace TechDrawGui
{
class MDIViewPage;
class Ui_TaskProjGroup;
class ViewProviderProjGroup;

class TaskProjGroup : public QWidget
{
    Q_OBJECT

public:
    TaskProjGroup(TechDraw::DrawProjGroup* featView, bool mode);
    ~TaskProjGroup() override = default;

    virtual bool accept();
    virtual bool reject();
    virtual bool apply();
    void modifyStandardButtons(QDialogButtonBox* box);
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel,
                     QPushButton* btnApply);

    void updateTask();
    std::pair<int, int> nearestFraction(double val, long int maxDenom = 999) const;
    // Sets the numerator and denominator widgets to match newScale
    void setFractionalScale(double newScale);
    void setCreateMode(bool mode) { m_createMode = mode;}
    bool getCreateMode() const { return m_createMode; }

protected:
    void changeEvent(QEvent *event) override;

    /// Connects and updates state of view checkboxes to match the state of multiView
    /*!
     * If addConnections is true, then also sets up Qt connections
     * between checkboxes and viewToggled()
     */
    void setupViewCheckboxes(bool addConnections = false);
    void setUiPrimary();
    void saveGroupState();
    void restoreGroupState();

    QString formatVector(Base::Vector3d vec);

protected Q_SLOTS:
    void viewToggled(bool toggle);

    /// Requests appropriate rotation of our DrawProjGroup
    void rotateButtonClicked();

    void projectionTypeChanged(QString qText);
    void scaleTypeChanged(int index);
    void AutoDistributeClicked(bool clicked);
    /// Updates item spacing
    void spacingChanged();
    void scaleManuallyChanged(int unused);

private:
    TechDraw::DrawPage* m_page;
    MDIViewPage* m_mdi;

    std::unique_ptr<Ui_TaskProjGroup> ui;
    TechDraw::DrawProjGroup* multiView;
    bool m_createMode;

    bool blockUpdate;
    /// Translate a view checkbox index into represented view string, depending on projection type
    const char * viewChkIndexToCStr(int index);

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;
    QPushButton* m_btnApply;

    std::vector<App::DocumentObject*> m_saveSource;
    std::string    m_saveProjType;
    std::string    m_saveScaleType;
    double         m_saveScale;
    bool           m_saveAutoDistribute;
    double         m_saveSpacingX;
    double         m_saveSpacingY;
    Base::Vector3d m_saveDirection;
    std::vector<std::string> m_saveViewNames;
};

class TaskDlgProjGroup : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgProjGroup(TechDraw::DrawProjGroup* featView, bool mode);
    ~TaskDlgProjGroup() override;

    const ViewProviderProjGroup * getViewProvider() const { return viewProvider; }
    TechDraw::DrawProjGroup * getMultiView() const { return multiView; }

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel; }
    void modifyStandardButtons(QDialogButtonBox* box) override;

    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
    { return false; }
    void setCreateMode(bool mode);

    void update();

protected:
    const ViewProviderProjGroup *viewProvider;
    TechDraw::DrawProjGroup *multiView;

private:
    TaskProjGroup * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKVIEWGROUP_H
