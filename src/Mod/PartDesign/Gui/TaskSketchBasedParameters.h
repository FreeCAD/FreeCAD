/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#ifndef GUI_TASKVIEW_TaskSketchBasedParameters_H
#define GUI_TASKVIEW_TaskSketchBasedParameters_H

#include <Gui/Selection.h>
#include "ViewProvider.h"

#include "TaskFeatureParameters.h"

namespace App {
class Property;
}

namespace PartDesignGui {


/// Convenience class to collect common methods for all SketchBased features
class TaskSketchBasedParameters : public PartDesignGui::TaskFeatureParameters,
                                  public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskSketchBasedParameters(PartDesignGui::ViewProvider* vp, QWidget *parent,
                              const std::string& pixmapname, const QString& parname);
    ~TaskSketchBasedParameters();

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg)=0;
    const QString onAddSelection(const Gui::SelectionChanges& msg);
    void onSelectReference(const bool pressed, const bool edge, const bool face, const bool planar);
    void exitSelectionMode();
    QVariant setUpToFace(const QString& text);
    /// Try to find the name of a feature with the given label.
    /// For faster access a suggested name can be tested, first.
    QVariant objectNameByLabel(const QString& label, const QVariant& suggest) const;

    static QString getFaceReference(const QString& obj, const QString& sub);
};

class TaskDlgSketchBasedParameters : public PartDesignGui::TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    TaskDlgSketchBasedParameters(PartDesignGui::ViewProvider *vp);
    ~TaskDlgSketchBasedParameters();

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskSketchBasedParameters_H
