/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef GUI_TASKVIEW_TaskSketcherMessages_H
#define GUI_TASKVIEW_TaskSketcherMessages_H

#include <boost_signals2.hpp>

#include <Gui/TaskView/TaskView.h>


class Ui_TaskSketcherMessages;
using Connection = boost::signals2::connection;

namespace App
{
class Property;
}

namespace SketcherGui
{

class ViewProviderSketch;

class TaskSketcherMessages: public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    explicit TaskSketcherMessages(ViewProviderSketch* sketchView);
    ~TaskSketcherMessages() override;

    void slotSetUp(const QString& state,
                   const QString& msg,
                   const QString& link,
                   const QString& linkText);

private:
    void setupConnections();
    void onLabelConstrainStatusLinkClicked(const QString&);
    void onAutoUpdateStateChanged();
    void onManualUpdateClicked(bool checked);

    void updateToolTip(const QString& link);

protected:
    ViewProviderSketch* sketchView;
    Connection connectionSetUp;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskSketcherMessages> ui;
};

}  // namespace SketcherGui

#endif  // GUI_TASKVIEW_TaskSketcherMessages_H
