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

#pragma once

#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/TechDraw/TechDrawGlobal.h>
#include "mrichtextedit.h"
#include "MDIViewPage.h"

class Ui_TaskRichAnno;

namespace TechDraw
{
class DrawPage;
class DrawView;
class DrawRichAnno;
}

namespace TechDrawGui
{
class QGIView;
class QGIPrimPath;
class QGIRichAnno;
class MDIViewPage;
class QGMText;
class ViewProviderRichAnno;
class ViewProviderPage;

class TaskRichAnno : public QWidget
{
    Q_OBJECT

public:
    TaskRichAnno(TechDraw::DrawView* baseFeat, TechDraw::DrawPage* page);
    explicit TaskRichAnno(TechDrawGui::ViewProviderRichAnno* annoVP);
    ~TaskRichAnno() override;

    void finishSetup();

    virtual bool accept();
    virtual bool reject();
    virtual void setCreateMode(bool mode) { m_createMode = mode; }
    virtual bool getCreateMode() { return m_createMode; }
    void updateTask();
    void saveButtons(QPushButton* btnOK, QPushButton* btnCancel);
    void enableTaskButtons(bool enable);

    ViewProviderRichAnno* getAnnoVP() const
    {
        return m_annoVP;
    }

protected:
    void changeEvent(QEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    void createAnnoFeature(const QPointF* scenePos = nullptr);
    void commonFeatureUpdate();

    QPointF calcTextStartPos(double scale);

    void setUiPrimary();
    void setUiEdit();
    void enableTextUi(bool enable);
    double prefWeight() const;
    Base::Color prefLineColor();

protected Q_SLOTS:
    void onMaxWidthChanged(double value);
    void onViewWidthChanged();
    void onShowFrameToggled(bool checked);
    void onFrameColorChanged();
    void onFrameWidthChanged(double value);
    void onFrameStyleChanged(int index);
    void onViewTransformed();
    void refocusAnnotation();

    void onViewSelectionChanged();
    void onViewPositionChanged(const QPointF& scenePos);

private:
    void removeViewFilter();
    void enterPlacementMode();
    void createAndSetupAnnotation(const QPointF* scenePos = nullptr);
    void createAnnoIfNotAlready();

    std::unique_ptr<Ui_TaskRichAnno> ui;

    ViewProviderPage* m_vpp;
    ViewProviderRichAnno* m_annoVP;
    TechDraw::DrawView* m_baseFeat;
    TechDraw::DrawPage* m_basePage;
    TechDraw::DrawRichAnno* m_annoFeat;
    QGIView* m_qgParent;
    std::string m_qgParentName;

    bool m_createMode;
    bool m_placementMode;

    bool m_inProgressLock;

    QPushButton* m_btnOK;
    QPushButton* m_btnCancel;

    QString m_title;

    QGIRichAnno* m_qgiAnno;
    bool m_syncLock;
    MDIViewPage* m_view;

    QPointer<MRichTextEdit> m_toolbar {nullptr};
    QPointer<QWidget> m_viewport {nullptr};
};

class TaskDlgRichAnno : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgRichAnno(TechDraw::DrawView* baseFeat, TechDraw::DrawPage* page);
    explicit TaskDlgRichAnno(TechDrawGui::ViewProviderRichAnno* annoVP);
    ~TaskDlgRichAnno() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    void autoClosedOnTransactionChange() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
    {
        return false;
    }

    void modifyStandardButtons(QDialogButtonBox* box) override;

    bool isFor(ViewProviderRichAnno* vp) const
    {
        return (widget->getAnnoVP() == vp);
    }

private:
    TaskRichAnno * widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui