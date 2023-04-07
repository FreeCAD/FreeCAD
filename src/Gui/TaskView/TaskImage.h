// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef GUI_TASKIMAGE_H
#define GUI_TASKIMAGE_H

#include <QPointer>
#include <Gui/TaskView/TaskDialog.h>
#include <App/DocumentObserver.h>
#include <App/ImagePlane.h>
#include <memory>
#include <vector>

class SbVec3f;
class SoEventCallback;
class SoSeparator;
class SoDatumLabel;

namespace Gui {

class View3DInventorViewer;
class ViewProvider;
class InteractiveScale : public QObject
{
    Q_OBJECT

public:
    explicit InteractiveScale(View3DInventorViewer* view, ViewProvider* vp, SbVec3f normal);
    ~InteractiveScale();
    void activate(bool allowOutside);
    void deactivate();
    bool isActive() const {
        return active;
    }
    double getDistance() const;
    double getDistance(const SbVec3f&) const;

private:
    static void getMouseClick(void * ud, SoEventCallback * ecb);
    static void getMousePosition(void * ud, SoEventCallback * ecb);
    void findPointOnPlane(SoEventCallback * ecb);
    void findPointOnImagePlane(SoEventCallback * ecb);
    void findPointOnFocalPlane(SoEventCallback * ecb);
    void collectPoint(const SbVec3f&);

Q_SIGNALS:
    void selectedPoints(size_t);

private:
    bool active;
    bool allowOutsideImage;
    SoSeparator* root;
    SoDatumLabel* measureLabel;
    QPointer<Gui::View3DInventorViewer> viewer;
    ViewProvider* viewProv;
    std::vector<SbVec3f> points;
    SbVec3f norm;
};

class Ui_TaskImage;
class TaskImage : public QWidget
{
    Q_OBJECT

public:
    explicit TaskImage(Image::ImagePlane* obj, QWidget* parent = nullptr);
    ~TaskImage() override;

    void open();
    void accept();
    void reject();

private:
    void initialiseTransparency();
    void connectSignals();

    void onInteractiveScale();
    View3DInventorViewer* getViewer() const;
    void selectedPoints(size_t num);
    void scaleImage(double);
    void startScale();
    void acceptScale();
    void rejectScale();
    SbVec3f getNorm();

    void restore(const Base::Placement&);
    void onPreview();
    void updateIcon();
    void updatePlacement();

private Q_SLOTS:
    void changeTransparency(int val);
    void changeWidth(double val);
    void changeHeight(double val);

private:
    std::unique_ptr<Ui_TaskImage> ui;
    QPointer<InteractiveScale> scale;
    App::WeakPtrT<Image::ImagePlane> feature;
    double aspectRatio;
};

class TaskImageDialog : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskImageDialog(Image::ImagePlane* obj);

public:
    void open() override;
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    TaskImage* widget;
};

}

#endif // GUI_TASKIMAGE_H
