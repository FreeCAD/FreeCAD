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

#include <Inventor/SbVec3f.h>
#include <QPointer>
#include <Gui/TaskView/TaskDialog.h>
#include <App/DocumentObserver.h>
#include <App/ImagePlane.h>
#include <memory>
#include <vector>

class SbVec3f;
class SoEventCallback;
class EditableDatumLabel;

namespace Gui {

class View3DInventorViewer;
class ViewProvider;
class InteractiveScale : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(InteractiveScale)

public:
    explicit InteractiveScale(View3DInventorViewer* view, ViewProvider* vp, const Base::Placement& plc);
    ~InteractiveScale() override;

    bool eventFilter(QObject* object, QEvent* event) override;
    void activate();
    void deactivate();
    bool isActive() const {
        return active;
    }
    double getScaleFactor() const;
    double getDistance(const SbVec3f&) const;
    void setPlacement(const Base::Placement& plc);

private:
    static void soEventFilter(void * ud, SoEventCallback * ecb);
    static void getMousePosition(void * ud, SoEventCallback * ecb);
    void findPointOnImagePlane(SoEventCallback * ecb);
    void collectPoint(const SbVec3f&);
    void setDistance(const SbVec3f&);

    /// give the coordinates of a line on the image plane in imagePlane (2D) coordinates
    SbVec3f getCoordsOnImagePlane(const SbVec3f& point);

Q_SIGNALS:
    void scaleRequired();
    void scaleCanceled();
    void enableApplyBtn();

private:
    bool active;
    Base::Placement placement;
    EditableDatumLabel* measureLabel;
    QPointer<Gui::View3DInventorViewer> viewer;
    ViewProvider* viewProv;
    std::vector<SbVec3f> points;
    SbVec3f midPoint;
};

class Ui_TaskImage;
class TaskImage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(TaskImage)

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
    void scaleImage(double);
    void startScale();
    void acceptScale();
    void rejectScale();
    void enableApplyBtn();

    void restore(const Base::Placement&);
    void restoreAngles(const Base::Rotation&);
    void onPreview();
    void updateIcon();
    void updatePlacement();

private:
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
