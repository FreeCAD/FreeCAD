/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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


#ifndef PARTGUI_ViewProviderDatum_H
#define PARTGUI_ViewProviderDatum_H

#include "Gui/ViewProviderGeometryObject.h"
#include <Base/BoundBox.h>

class SoPickStyle;
class SbBox3f;
class SoGetBoundingBoxAction;

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderDatum : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderDatum);

public:
    /// constructor
    ViewProviderDatum();
    /// destructor
    virtual ~ViewProviderDatum();

    /// grouping handling
    void setupContextMenu(QMenu*, QObject*, const char*);

    virtual void attach(App::DocumentObject *);
    virtual bool onDelete(const std::vector<std::string> &);
    virtual bool doubleClicked(void);
    std::vector<std::string> getDisplayModes(void) const;
    void setDisplayMode(const char* ModeName);

    /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel(void) const { return true; }
    /// indicates if the ViewProvider can be selected
    virtual bool isSelectable(void) const ;
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail *) const;
    virtual SoDetail* getDetail(const char*) const;

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
    virtual void setExtents (Base::BoundBox3d /*bbox*/)
        { }

    /// Update the visual sizes. This overloaded version of the previous function to allow pass coin type
    void setExtents (const SbBox3f &bbox);

    /// update size to match the guessed bounding box
    void updateExtents ();

    /// The datum type (Plane, Line or Point)
    // TODO remove this attribute (2015-09-08, Fat-Zer)
    QString datumType;
    QString datumText;

    /**
     * Computes appropriate bounding box for the given list of objects to be passed to setExtents ()
     * @param bboxAction  a coin action for traverse the given objects views.
     * @param objs        the list of objects to traverse, due to we traverse the scene graph, the geo children
     *                    will likely be traveresed too.
     */
    static SbBox3f getRelevantBoundBox (
            SoGetBoundingBoxAction &bboxAction,
            const std::vector <App::DocumentObject *> &objs);

    /// Default size used to produce the default bbox
    static const double defaultSize;

    // Returned default bounding box if relevant is can't be used for some reason
    static SbBox3f defaultBoundBox ();

    // Returns a default margin factor (part of size )
    static double marginFactor () { return 0.1; };

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);

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
    SoSeparator *getShapeRoot () { return pShapeSep; }

    virtual QIcon mergeOverlayIcons (const QIcon & orig) const override;

private:
    SoSeparator* pShapeSep;
    SoPickStyle* pPickStyle;
    std::string oldWb;
    App::DocumentObject* oldTip;

};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderDatum_H
