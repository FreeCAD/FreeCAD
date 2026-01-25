/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


class Ui_TaskSectionView;

namespace TechDraw {
    class DrawViewPart;
    class DrawViewSection;
}

namespace TechDrawGui
{

class CompassWidget;
class VectorEditWidget;

class TaskSectionView : public QWidget
{
    Q_OBJECT

public:
    explicit TaskSectionView(TechDraw::DrawViewPart* base);
    explicit TaskSectionView(TechDraw::DrawViewSection* section);
    ~TaskSectionView() override = default;

    virtual bool accept();
    virtual bool reject();

protected:
    void changeEvent(QEvent *event) override;
    void saveSectionState();
    void restoreSectionState();

    bool apply(bool forceUpdate = false);
    void applyQuick(std::string dir);
    void applyAligned();

    TechDraw::DrawViewSection* createSectionView();
    void updateSectionView();

    void setUiPrimary();
    void setUiEdit();
    void setUiCommon(Base::Vector3d origin);

    void checkAll(bool check);
    void enableAll(bool enable);

    void failNoObject();
    bool isBaseValid();
    bool isSectionValid();

protected Q_SLOTS:
    void onUpClicked();
    void onDownClicked();
    void onLeftClicked();
    void onRightClicked();
    void onIdentifierChanged();
    void onScaleChanged();
    void onXChanged();
    void onYChanged();
    void onZChanged();
    void scaleTypeChanged(int index);
    void liveUpdateClicked();
    void updateNowClicked();
    void slotChangeAngle(double newAngle);
    void slotViewDirectionChanged(Base::Vector3d newDirection);

private:
    double requiredRotation(double inputAngle);
    std::string makeSectionLabel(QString symbol);
    bool directionChanged() const { return m_directionChanged; }
    void directionChanged(bool newState) { m_directionChanged = newState; }

    std::unique_ptr<Ui_TaskSectionView> ui;
    TechDraw::DrawViewPart* m_base;
    TechDraw::DrawViewSection* m_section;
    std::string m_symbol;
    Base::Vector3d m_normal;
    Base::Vector3d m_direction;
    Base::Vector3d m_origin;

    std::string m_saveSymbol;
    std::string m_saveDirName;
    Base::Vector3d m_saveNormal;
    Base::Vector3d m_saveDirection;
    Base::Vector3d m_saveOrigin;
    double m_saveScale;
    int m_saveScaleType;

    std::string m_dirName;
    std::string m_sectionName;
    std::string m_baseName;
    App::Document* m_doc;

    bool m_createMode;
    bool m_saved;

    std::string m_saveBaseName;
    std::string m_savePageName;

    int m_applyDeferred;
    CompassWidget* m_compass;
    VectorEditWidget* m_viewDirectionWidget;
    bool m_directionIsSet;
    bool m_modelIsDirty;

    bool m_scaleEdited;
    bool m_directionChanged{false};
};

class TaskDlgSectionView : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgSectionView(TechDraw::DrawViewPart* base);
    explicit TaskDlgSectionView(TechDraw::DrawViewSection* section);
    ~TaskDlgSectionView() override;

    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
/*    virtual void clicked(int);*/
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

    bool isAllowedAlterSelection() const override
    { return false; }
    bool isAllowedAlterDocument() const override
    { return false; }

private:
    TaskSectionView * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui