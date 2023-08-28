/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAWGUI_TASKCENTERLINE_H
#define TECHDRAWGUI_TASKCENTERLINE_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Mod/TechDraw/App/CenterLine.h>
#include <Mod/TechDraw/App/Cosmetic.h>


namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawViewPart;
class CosmeticEdge;
class Face;
class LineFormat;
class CenterLine;
}

namespace TechDrawGui
{
class QGSPage;
class QGVPage;
class QGIView;
class QGIPrimPath;
class MDIViewPage;
class ViewProviderViewPart;
class Ui_TaskCenterLine;

class TaskCenterLine : public QWidget
{
    Q_OBJECT

public:
    TaskCenterLine(TechDraw::DrawViewPart* partFeat,
                   TechDraw::DrawPage* page,
                   std::vector<std::string> subNames,
                   bool editMode);
    TaskCenterLine(TechDraw::DrawViewPart* partFeat,
                   TechDraw::DrawPage* page,
                   std::string edgeName,
                   bool editMode);
    ~TaskCenterLine() override;

    virtual bool accept();
    virtual bool reject();
    virtual void setCreateMode(bool mode) { m_createMode = mode; }
    virtual bool getCreateMode() const { return m_createMode; }
    void updateTask();
    void saveButtons(QPushButton* btnOK,
                     QPushButton* btnCancel);
    void enableTaskButtons(bool isEnabled);

protected:
    void changeEvent(QEvent *event) override;
    void setUiConnect();
    void setUiPrimary();
    void setUiEdit();
    void createCenterLine();
    void updateOrientation();

    double getCenterWidth();
    QColor getCenterColor();
    Qt::PenStyle getCenterStyle();
    double getExtendBy();

    int checkPathologicalEdges(int inMode);
    int checkPathologicalVertices(int inMode);
    void setUiOrientation(int orientation);

private:
    std::unique_ptr<Ui_TaskCenterLine> ui;

    TechDraw::DrawViewPart* m_partFeat;
    TechDraw::DrawPage* m_basePage;
    bool m_createMode;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    std::vector<std::string> m_subNames;
    std::string m_edgeName;
    int m_geomIndex;
    TechDraw::CenterLine* m_cl;
    TechDraw::CenterLine orig_cl;
    int m_type;
    int m_mode;
    bool m_editMode;

private Q_SLOTS:
    void onOrientationChanged();
    void onShiftHorizChanged();
    void onShiftVertChanged();
    void onRotationChanged();
    void onExtendChanged();
    void onColorChanged();
    void onWeightChanged();
    void onStyleChanged();

};

class TaskDlgCenterLine : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                      TechDraw::DrawPage* page,
                      std::vector<std::string> subNames,
                      bool editMode);
    TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                      TechDraw::DrawPage* page,
                      std::string edgeName,
                      bool editMode);
    ~TaskDlgCenterLine() override;

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
    void helpRequested() override { return;}
    bool isAllowedAlterDocument() const override
                        { return false; }
    void update();

    void modifyStandardButtons(QDialogButtonBox* box) override;

protected:

private:
    TaskCenterLine* widget;
    Gui::TaskView::TaskBox* taskbox;

};

} //namespace TechDrawGui

#endif // #ifndef TECHDRAWGUI_TASKCENTERLINE_H
