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

#pragma once

#include <QString>

#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


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
class CompassWidget;
class VectorEditWidget;

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
    TaskComplexSection(TechDraw::DrawComplexSection* complexSection);
    ~TaskComplexSection() override = default;

    virtual bool accept();
    virtual bool reject();

protected:
    void changeEvent(QEvent *event) override;
    void saveSectionState();
    void restoreSectionState();

    bool apply(bool forceUpdate = false);
    void applyQuick(std::string dir);
    void applyAligned();

    void setUiPrimary();
    void setUiEdit();
    void setUiCommon();

    void checkAll(bool check);
    void enableAll(bool enable);

    void failNoObject();
    bool isBaseValid();
    bool isSectionValid();

    void updateUi();

protected Q_SLOTS:
    void onSectionObjectsUseSelectionClicked();
    void onProfileObjectsUseSelectionClicked();
    void onUpClicked();
    void onDownClicked();
    void onLeftClicked();
    void onRightClicked();
    void onIdentifierChanged();
    void onScaleChanged();
    void scaleTypeChanged(int index);
    void liveUpdateClicked();
    void updateNowClicked();
    void slotChangeAngle(double newAngle);
    void slotViewDirectionChanged(Base::Vector3d newDirection);

private:
    double requiredRotation(double inputAngle);
    std::string makeSectionLabel(const QString& symbol);

    void createComplexSection();
    void updateComplexSection();

    QString sourcesToString();
    std::unique_ptr<Ui_TaskComplexSection> ui;

    TechDraw::DrawPage* m_page;
    App::Document* m_doc;
    TechDraw::DrawViewPart* m_baseView;
    TechDraw::DrawComplexSection* m_section;
    std::vector<App::DocumentObject*> m_shapes;
    std::vector<App::DocumentObject*> m_xShapes;
    App::DocumentObject* m_profileObject;
    std::vector<std::string> m_profileSubs;
    std::string m_dirName;
    std::string m_sectionName;
    Base::Vector3d m_saveNormal;
    Base::Vector3d m_saveXDir;
    std::string m_saveBaseName;
    std::string m_savePageName;
    std::string m_saveSymbol;
    std::string m_saveDirName;
    Base::Vector3d m_saveDirection;
    Base::Vector3d m_saveOrigin;
    double m_saveScale;
    int m_saveScaleType;
    bool m_saved;
    bool m_createMode;
    Base::Vector3d m_normal;

    int m_applyDeferred;
    CompassWidget* m_compass;
    double m_angle;
    VectorEditWidget* m_viewDirectionWidget;
    bool m_directionIsSet;
    bool m_modelIsDirty;

    bool m_scaleEdited;

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
    explicit TaskDlgComplexSection(TechDraw::DrawComplexSection* page) ;
    ~TaskDlgComplexSection() override = default;

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
                        { return false; }
    void update();

protected:

private:
    TaskComplexSection * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui