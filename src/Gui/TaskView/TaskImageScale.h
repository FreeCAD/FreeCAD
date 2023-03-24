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

#include <Gui/TaskView/TaskDialog.h>
#include <App/DocumentObserver.h>
#include <App/ImagePlane.h>
#include <memory>

namespace Gui {

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

private:
    std::unique_ptr<Ui_TaskImageScale> ui;
    App::WeakPtrT<Image::ImagePlane> feature;
    double aspectRatio;
};

}

#endif // GUI_TASKIMAGESCALE_H
