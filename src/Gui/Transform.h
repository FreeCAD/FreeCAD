/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_TRANSFORM_H
#define GUI_TRANSFORM_H

#include <QDialog>
#include <set>
#include <Base/Placement.h>

#include "Selection.h"
#include "TaskView/TaskDialog.h"
#include "TaskView/TaskView.h"


namespace Gui {
namespace Dialog {

class GuiExport TransformStrategy
{
public:
    TransformStrategy();
    virtual ~TransformStrategy();

    Base::Vector3d getRotationCenter() const;
    void acceptDataTransform(const Base::Matrix4D& mat, App::DocumentObject* obj);
    void commitTransform(const Base::Matrix4D& plm);
    void applyTransform(const Base::Placement& plm);
    void resetTransform();
    void applyViewTransform(const Base::Placement& plm, App::DocumentObject* obj);
    void resetViewTransform(App::DocumentObject* obj);
    virtual std::set<App::DocumentObject*> transformObjects() const = 0;
};

class GuiExport DefaultTransformStrategy : public TransformStrategy,
                                           public Gui::SelectionObserver
{
public:
    DefaultTransformStrategy(QWidget* widget);
    ~DefaultTransformStrategy() override;
    std::set<App::DocumentObject*> transformObjects() const override;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    std::set<App::DocumentObject*> selection;
    QWidget* widget;
};

class Ui_Placement;
class GuiExport Transform : public QDialog
{
    Q_OBJECT

public:
    Transform(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~Transform() override;
    void accept() override;
    void reject() override;
    void showStandardButtons(bool);
    void setTransformStrategy(TransformStrategy* ts);

protected:
    Base::Vector3d getDirection() const;
    void changeEvent(QEvent *e) override;

public:
    void onApplyButtonClicked();

private:
    void onTransformChanged(int);
    Base::Placement getPlacementData() const;

private:
    Ui_Placement* ui;
    Base::Placement pm;
    std::set<App::DocumentObject*> selection;
    TransformStrategy* strategy;
};

class GuiExport TaskTransform : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskTransform();
    ~TaskTransform() override;

public:
    bool accept() override;
    bool reject() override;
    void clicked(int) override;

    void setTransformStrategy(TransformStrategy* ts);
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok |
             QDialogButtonBox::Apply |
             QDialogButtonBox::Cancel; }

private:
    Transform* dialog;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_TRANSFORM_H
