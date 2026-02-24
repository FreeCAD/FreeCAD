// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

namespace Gui
{
class Document;
class ViewProvider;
}  // namespace Gui

namespace Materials
{
class Material;
}

namespace PartGui
{

class ViewProviderPartExt;

class FaceAppearances: public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit FaceAppearances(ViewProviderPartExt* vp, QWidget* parent = nullptr);
    ~FaceAppearances() override;

    void open();
    bool accept();
    bool reject();

private:
    void setupConnections();
    void onMaterialSelected(const std::shared_ptr<Materials::Material>& material);
    void onDefaultButtonClicked();
    void onBoxSelectionToggled(bool checked);
    void onButtonCustomAppearanceClicked();

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void changeEvent(QEvent* e) override;
    void slotUndoDocument(const Gui::Document& Doc);
    void slotDeleteDocument(const Gui::Document&);
    void slotDeleteObject(const Gui::ViewProvider&);
    void updatePanel();
    int getFirstIndex() const;

private:
    class Private;
    Private* d;
};

class TaskFaceAppearances: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskFaceAppearances(ViewProviderPartExt* vp);
    ~TaskFaceAppearances() override;

public:
    void open() override;
    bool accept() override;
    bool reject() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    FaceAppearances* widget;
};

}  // namespace PartGui
