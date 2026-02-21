// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Gui/TaskView/TaskView.h>

class Ui_TaskTransformedMessages;
using Connection = fastsignals::connection;

namespace App
{
class Property;
}

namespace PartDesignGui
{

class ViewProviderTransformed;

class TaskTransformedMessages: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskTransformedMessages(ViewProviderTransformed* transformedView);
    ~TaskTransformedMessages() override;

    void slotDiagnosis(QString msg);

private Q_SLOTS:

protected:
    ViewProviderTransformed* transformedView;
    Connection connectionDiagnosis;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskTransformedMessages> ui;
};

}  // namespace PartDesignGui
