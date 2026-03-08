// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender                                   *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
 ***************************************************************************/


#pragma once

#include "Gui/ViewProviderGeometryObject.h"
#include <Base/BoundBox.h>
#include <QCoreApplication>

#include <Mod/Part/Gui/ViewProviderAttachExtension.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

class SoPickStyle;
class SbBox3f;
class SoGetBoundingBoxAction;

namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderDatum: public Gui::ViewProviderGeometryObject,
                                             PartGui::ViewProviderAttachExtension
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderDatum)
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesignGui::ViewProviderDatum);

public:
    /// constructor
    ViewProviderDatum();
    /// destructor
    ~ViewProviderDatum() override;

    /// grouping handling
    void setupContextMenu(QMenu*, QObject*, const char*) override;

    void attach(App::DocumentObject*) override;
    bool onDelete(const std::vector<std::string>&) override;
    bool doubleClicked() override;
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;

    /// indicates if the ViewProvider use the new Selection model
    bool useNewSelectionModel() const override
    {
        return true;
    }
    /// indicates if the ViewProvider can be selected
    bool isSelectable() const override;
    /// return a hit element to the selection path or 0
    std::string getElement(const SoDetail*) const override;
    SoDetail* getDetail(const char*) const override;

    /**
     * Enable/Disable the selectability of the datum
     * This differs from the normal ViewProvider selectability in that, that with this enabled one
     * can pick through the datum and select stuff behind it.
     */
    bool isPickable();
    void setPickable(bool val);

    /**
     * Update the visual size to match the given extents
     * @note should be reimplemented in the offspings
     * @note use FreeCAD-specific bbox here to simplify the math in derived classes
     */
    virtual void setExtents(Base::BoundBox3d /*bbox*/)
    {}

    /// Update the visual sizes. This overloaded version of the previous function to allow pass coin type
    void setExtents(const SbBox3f& bbox);

    /// update size to match the guessed bounding box
    void updateExtents();

    /// The datum type (Plane, Line or Point)
    // TODO remove this attribute (2015-09-08, Fat-Zer)
    QString datumType;
    QString datumText;
    QString datumMenuText;

    /**
     * Computes appropriate bounding box for the given list of objects to be passed to setExtents ()
     * @param bboxAction  a coin action for traverse the given objects views.
     * @param objs        the list of objects to traverse, due to we traverse the scene graph, the
     * geo children will likely be traversed too.
     */
    static SbBox3f getRelevantBoundBox(
        SoGetBoundingBoxAction& bboxAction,
        const std::vector<App::DocumentObject*>& objs
    );

    /// Default size used to produce the default bbox
    static const double defaultSize;

    // Returned default bounding box if relevant is can't be used for some reason
    static SbBox3f defaultBoundBox();

    // Returns a default margin factor (part of size )
    static double marginFactor()
    {
        return 0.1;
    };

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;

    /**
     * Guesses the context this datum belongs to and returns appropriate bounding box of all
     *  visible content of the feature
     *
     * Currently known contexts are:
     *  - PartDesign::Body
     *  - App::DocumentObjectGroup (App::Part as well as subclass)
     *  - Whole document
     */
    SbBox3f getRelevantBoundBox() const;

    // Get the separator to fill with datum content
    SoSeparator* getShapeRoot()
    {
        return pShapeSep;
    }

private:
    SoSeparator* pShapeSep;
    SoPickStyle* pPickStyle;
    std::string oldWb;
    App::DocumentObject* oldTip;
};

}  // namespace PartDesignGui
