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

#ifndef GUI_TASKIMAGESCALE_H
#define GUI_TASKIMAGESCALE_H

#include <QPointer>
#include <Gui/TaskView/TaskDialog.h>
#include <App/DocumentObserver.h>
#include <App/ImagePlane.h>
#include <memory>
#include <vector>

class SbVec3f;
class SoEventCallback;
class SoCoordinate3;
class SoSeparator;
class SoLineSet;

namespace Gui {

class View3DInventorViewer;
class ViewProvider;
class InteractiveScale : public QObject
{
    Q_OBJECT

public:
    explicit InteractiveScale(View3DInventorViewer* view, ViewProvider* vp);
    ~InteractiveScale();
    void activate(bool allowOutside);
    void deactivate();
    bool isActive() const {
        return active;
    }
    double getDistance() const;
    double getDistance(const SbVec3f&) const;
    void clearPoints();

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
    SoCoordinate3* coords;
    SoSeparator* root;
    QPointer<Gui::View3DInventorViewer> viewer;
    ViewProvider* viewProv;
    std::vector<SbVec3f> points;
};

class Ui_TaskImageScale;
class TaskImageScale : public QWidget
{
    Q_OBJECT

public:
    explicit TaskImageScale(Image::ImagePlane* obj, QWidget* parent = nullptr);
    ~TaskImageScale() override;

private:
    void changeWidth();
    void changeHeight();
    void onInteractiveScale();
    View3DInventorViewer* getViewer() const;
    void selectedPoints(size_t num);
    void scaleImage(double);
    void startScale();
    void acceptScale();
    void rejectScale();

private:
    std::unique_ptr<Ui_TaskImageScale> ui;
    QPointer<InteractiveScale> scale;
    App::WeakPtrT<Image::ImagePlane> feature;
    double aspectRatio;
};

}

#endif // GUI_TASKIMAGESCALE_H
