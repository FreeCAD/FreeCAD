/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <QRectF>

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <Base/BoundBox.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawViewCollection.h"


class gp_Dir;
class gp_Pnt;

namespace TechDraw
{
const int MAXPROJECTIONCOUNT = 10;

class DrawProjGroupItem;
enum class ProjDirection : int;
enum class SpinDirection : int;
enum class RotationMotion : int;

/**
 * Class super-container for managing a collection of DrawProjGroupItem
 * Page Features
 */
class TechDrawExport DrawProjGroup : public TechDraw::DrawViewCollection
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawProjGroup);

public:
    /// Constructor
    DrawProjGroup();
    ~DrawProjGroup() override = default;

    App::PropertyLinkList   Source;
    App::PropertyXLinkList  XSource;

    App::PropertyEnumeration ProjectionType;

    /// Whether projcetion group view are automatically distributed or not
    App::PropertyBool AutoDistribute;
    /// Default horizontal spacing between adjacent views on Drawing, in mm
    App::PropertyLength spacingX;
    /// Default vertical spacing between adjacent views on Drawing, in mm
    App::PropertyLength spacingY;

    App::PropertyLink Anchor; /// Anchor Element to align views to

    // this needs to be kept in the same sequence as the literals in the cpp file and with the QComboBox
    // in TaskProjGroup.ui.
    enum class ViewProjectionConvention {
        FirstAngle = 0,
        ThirdAngle,
        Page
    };

    double autoScale() const override;
    double autoScale(double w, double h) const override;
    QRectF getRect() const override;   //always scaled
    QRectF getRect(bool scaled) const;     //scaled or unscaled

    /// Check if container has a view of a specific type
    bool hasProjection(const char *viewProjType) const;
    ///check if it is safe to delete item
    bool canDelete(const char *viewProjType) const;
    App::DocumentObject * getProjObj(const char *viewProjType) const;
    DrawProjGroupItem* getProjItem(const char *viewProjType) const;

    //! Adds a projection to the group
    /*!
     * \return pointer to the new view
     */
    App::DocumentObject * addProjection(const char *viewProjType);

    //! Removes a projection from the group
    /*!
     * \return number of projections remaining
     */
    int removeProjection(const char *viewProjType);

    int purgeProjections();
    Base::Vector3d getXYPosition(const char *viewTypeCStr);

    short mustExecute() const override;
    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderProjGroup";
    }
    //return PyObject as DrawProjGroupPy
    PyObject *getPyObject() override;

    /// Determines either "First angle" or "Third angle".
    App::Enumeration usedProjectionType();

    /// Allowed projection types - either Document, First angle or Third angle
    static const char* ProjectionTypeEnums[];

    bool hasAnchor();
    void setAnchorDirection(Base::Vector3d dir);
    Base::Vector3d getAnchorDirection();
    TechDraw::DrawProjGroupItem* getAnchor();
    std::pair<Base::Vector3d, Base::Vector3d> getDirsFromFront(DrawProjGroupItem* view);
    std::pair<Base::Vector3d, Base::Vector3d> getDirsFromFront(TechDraw::ProjDirection viewType);

    void updateSecondaryDirs();

    void rotate(const TechDraw::RotationMotion& motion);
    void spin(const TechDraw::SpinDirection& spindirection);
    void spin(double angle);

    void dumpISO(const char * title);
    std::vector<DrawProjGroupItem*> getViewsAsDPGI();

    void recomputeChildren();
    void updateChildrenScale();
    void autoPositionChildren();
    void updateChildrenEnforce();

    std::vector<App::DocumentObject*> getAllSources() const;
    bool checkFit() const override;
    bool checkFit(DrawPage* p) const override;

    bool waitingForChildren() const;
    void reportReady();

    void dumpTouchedProps();

protected:
    void unsetupObject() override;
    void onChanged(const App::Property* prop) override;

    /// Annoying helper - keep in sync with DrawProjGroupItem::TypeEnums
    /*!
     * \todo {See note regarding App::PropertyEnumeration on my wiki page https://freecad.org/wiki/User:Ian.rees}
     * \return true iff 'in' is a valid name for an orthographic/isometric view
     */
    bool checkViewProjType(const char *in);

    void arrangeViewPointers(std::array<DrawProjGroupItem*, MAXPROJECTIONCOUNT>& viewPtrs) const;

    /// Populates array of 10 BoundBox3d's given DrawProjGroupItem *s
    /*!
     * If documentScale is set, then returned bounding boxes are scaled as in
     * the Drawing.  Otherwise, the dimensions are as in object space.
     */
    void makeViewBbs(std::array<DrawProjGroupItem*, MAXPROJECTIONCOUNT>& viewPtrs,
                     std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT>& bboxes,
                     bool scaled = true) const;

    /// Helper for calculateAutomaticScale
    /*!
     * Returns a width and height in object-space scale, for the enabled views
     * without accounting for their actual X and Y positions or borders.
     */
    void getViewArea(std::array<TechDraw::DrawProjGroupItem *, MAXPROJECTIONCOUNT>& viewPtrs,
                        double &width, double &height,
                        bool scaled = true) const;

    /// Returns pointer to our page, or NULL if it couldn't be located
    TechDraw::DrawPage * getPage() const;

    void updateChildrenSource();
    void updateChildrenLock();

    int getViewIndex(const char *viewTypeCStr) const;
    int getDefProjConv() const;
    Base::Vector3d dir2vec(gp_Dir d);
    gp_Dir vec2dir(Base::Vector3d v);

    void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop) override;

    double getMaxRowHeight(std::array<int, 3> list,
                           std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT> bboxes);
    double getMaxColWidth(std::array<int, 3> list,
                          std::array<Base::BoundBox3d, MAXPROJECTIONCOUNT> bboxes);
};

} //namespace TechDraw