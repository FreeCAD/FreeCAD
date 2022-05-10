/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#ifndef FEM_ViewProviderAnalysis_H
#define FEM_ViewProviderAnalysis_H

#include <Gui/ViewProviderDocumentObjectGroup.h>
#include <Gui/ViewProviderPythonFeature.h>
#include <Mod/Fem/FemGlobal.h>
#include <QCoreApplication>


namespace FemGui
{

class FemGuiExport ViewProviderFemAnalysis : public Gui::ViewProviderDocumentObjectGroup
{
    Q_DECLARE_TR_FUNCTIONS(FemGui::ViewProviderFemAnalysis)
    PROPERTY_HEADER(FemGui::ViewProviderAnalysis);

public:
    /// constructor.
    ViewProviderFemAnalysis();

    /// destructor.
    virtual ~ViewProviderFemAnalysis();

    virtual bool doubleClicked(void);

    virtual std::vector<App::DocumentObject*> claimChildren(void)const;

    // handling when object is deleted
    virtual bool onDelete(const std::vector<std::string>&);
    /// Asks the view provider if the given object can be deleted.
    virtual bool canDelete(App::DocumentObject* obj) const;

    //virtual std::vector<App::DocumentObject*> claimChildren3D(void)const;
    void setupContextMenu(QMenu*, QObject*, const char*);

    /// A list of all possible display modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    // shows solid in the tree
    virtual bool isShow(void) const {
        return Visibility.getValue();
    }
    /// Hide the object in the view
    virtual void hide(void);
    /// Show the object in the view
    virtual void show(void);

    /** @name Drag and drop */
    //@{
    /// Returns true if the view provider generally supports dragging objects
    bool canDragObjects() const;
    /// Check whether the object can be removed from the view provider by drag and drop
    bool canDragObject(App::DocumentObject*) const;
    /// Starts to drag the object
    void dragObject(App::DocumentObject*);
    /// Returns true if the view provider generally accepts dropping of objects
    bool canDropObjects() const;
    /// Check whether the object can be dropped to the view provider by drag and drop
    bool canDropObject(App::DocumentObject*) const;
    /// If the dropped object type is accepted the object will be added as child
    void dropObject(App::DocumentObject*);
    //@}

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);

};

typedef Gui::ViewProviderPythonFeatureT<ViewProviderFemAnalysis> ViewProviderFemAnalysisPython;

} //namespace FemGui


#endif // FEM_ViewProviderAnalysis_H
