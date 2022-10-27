/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAWGUI_TASKCOMPLEXSECTION_H
#define TECHDRAWGUI_TASKCOMPLEXSECTION_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QString>

#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace App
{
class DocumentObject;
}

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
class DrawComplexSection;
}

namespace TechDrawGui
{
class Ui_TaskComplexSection;

class TaskComplexSection : public QWidget
{
    Q_OBJECT

public:
    TaskComplexSection(TechDraw::DrawPage* page,
                       TechDraw::DrawViewPart* baseView,
                       std::vector<App::DocumentObject*> shapes,
                       std::vector<App::DocumentObject*> xShapes,
                       App::DocumentObject* profileObject,
                       std::vector<std::string> profileSubs);
    ~TaskComplexSection() = default;

    virtual bool accept();
    virtual bool reject();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool button);
public Q_SLOTS:
    void onSectionObjectsUseSelectionClicked();
    void onProfileObjectsUseSelectionClicked();

protected:
    void changeEvent(QEvent *event) override;

    void setUiPrimary();
    void updateUi();

private:
    void createComplexSection();
    QString sourcesToString();
    std::unique_ptr<Ui_TaskComplexSection> ui;

    TechDraw::DrawPage* m_page;
    TechDraw::DrawViewPart* m_baseView;
    TechDraw::DrawComplexSection* m_section;
    std::vector<App::DocumentObject*> m_shapes;
    std::vector<App::DocumentObject*> m_xShapes;
    App::DocumentObject* m_profileObject;
    std::vector<std::string> m_profileSubs;
    std::string m_sectionName;
    Base::Vector3d m_saveXDir;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

};

class TaskDlgComplexSection : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgComplexSection(TechDraw::DrawPage* page,
                          TechDraw::DrawViewPart* baseView,
                          std::vector<App::DocumentObject*> shapes,
                          std::vector<App::DocumentObject*> xShapes,
                          App::DocumentObject* profileObject,
                          std::vector<std::string> profileSubs);
    ~TaskDlgComplexSection() override;

public:
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
    void update();

    void modifyStandardButtons(QDialogButtonBox* box) override;

protected:

private:
    TaskComplexSection * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKCOMPLEXSECTION_H
