/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#ifndef TASKFEATUREPARAMETERS_H_NAHKE2YZ
#define TASKFEATUREPARAMETERS_H_NAHKE2YZ


#include <Gui/TaskView/TaskDialog.h>

#include "ViewProvider.h"

namespace PartDesignGui { 

/// A common base for sketch based, dressup and other solid parameters dialogs
class TaskDlgFeatureParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgFeatureParameters(PartDesignGui::ViewProvider *vp);
    ~TaskDlgFeatureParameters();

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();

protected:
    PartDesignGui::ViewProvider   *vp;
};

} //namespace PartDesignGui


#endif /* end of include guard: TASKFEATUREPARAMETERS_H_NAHKE2YZ */
