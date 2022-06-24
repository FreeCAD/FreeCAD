/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef PARTGUI_ViewProviderTransformed_H
#define PARTGUI_ViewProviderTransformed_H

#include "ViewProvider.h"

namespace PartDesignGui {

class TaskDlgTransformedParameters;

class PartDesignGuiExport ViewProviderTransformed : public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderTransformed);

public:
    /// constructor
    ViewProviderTransformed()
        : featureName("undefined"), pcRejectedRoot(nullptr) {}
    /// destructor
    virtual ~ViewProviderTransformed()
        {}

    void setupContextMenu(QMenu*, QObject*, const char*) override;

    virtual bool onDelete(const std::vector<std::string> &) override;

    /// signals if the transformation contains errors
    boost::signals2::signal<void (QString msg)> signalDiagnosis;

    // The feature name of the subclass
    std::string featureName;
    // Name of menu dialog
    QString menuName;

    virtual Gui::ViewProvider *startEditing(int ModNum=0) override;

protected:
    virtual bool setEdit(int ModNum) override;
    virtual void unsetEdit(int ModNum) override;

    bool checkDlgOpen(TaskDlgTransformedParameters* transformedDlg);

    // node for the representation of rejected repetitions
    SoGroup           * pcRejectedRoot;

    QString diagMessage;

public:
    void recomputeFeature(bool recompute=true);
    QString getMessage() const {return diagMessage;}

private:
    void showRejectedShape(TopoDS_Shape shape);
};


} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderTransformed_H
