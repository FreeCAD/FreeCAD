/***************************************************************************
 *   Copyright (c) 2017 WandererFan <wandererfan@gmail.com>                *
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

#ifndef GUI_TASKVIEW_TASKGEOMHATCH_H
#define GUI_TASKVIEW_TASKGEOMHATCH_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace App
{
class DocumentObject;
}

namespace TechDraw
{
class DrawGeomHatch;
}


namespace TechDrawGui
{
class Ui_TaskGeomHatch;
class ViewProviderGeomHatch;

class TaskGeomHatch : public QWidget
{
    Q_OBJECT

public:
    TaskGeomHatch(TechDraw::DrawGeomHatch* inHatch, TechDrawGui::ViewProviderGeomHatch* inVp, bool mode);
    ~TaskGeomHatch() override = default;

    virtual bool accept();
    virtual bool reject();
    void setCreateMode(bool mode) { m_createMode = mode;}
    bool getCreateMode() const { return m_createMode; }

protected:
    void changeEvent(QEvent *event) override;
    void initUi();
    void updateValues();
    void getParameters();
    QStringList listToQ(std::vector<std::string> inList);

protected Q_SLOTS:
    void onFileChanged();

private:
    std::unique_ptr<Ui_TaskGeomHatch> ui;
    TechDraw::DrawGeomHatch* m_hatch;
    TechDrawGui::ViewProviderGeomHatch* m_Vp;
    App::DocumentObject* m_source;
    std::string m_file;
    std::string m_name;
    double m_scale;
    double m_weight;
    App::Color m_color;
    std::string m_origFile;
    std::string m_origName;
    double m_origScale;
    double m_origWeight;
    App::Color m_origColor;
    double m_rotation;
    double m_origRotation;
    Base::Vector3d m_offset;
    Base::Vector3d m_origOffset;

    bool m_createMode;

private Q_SLOTS:
        void onNameChanged();
        void onScaleChanged();
        void onLineWeightChanged();
        void onColorChanged();
        void onRotationChanged();
        void onOffsetChanged();

};

class TaskDlgGeomHatch : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgGeomHatch(TechDraw::DrawGeomHatch* inHatch, TechDrawGui::ViewProviderGeomHatch* inVp, bool mode);
    ~TaskDlgGeomHatch() override;
    const ViewProviderGeomHatch * getViewProvider() const { return viewProvider; }

    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    bool isAllowedAlterDocument() const override
    { return false; }
    void setCreateMode(bool mode);

    void update();

private:
    const ViewProviderGeomHatch *viewProvider;
    TaskGeomHatch * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui

#endif // #ifndef GUI_TASKVIEW_TASKGEOMHATCH_H
