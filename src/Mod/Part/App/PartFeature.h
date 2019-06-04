/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#ifndef PART_FEATURE_H
#define PART_FEATURE_H

#include "TopoShape.h"
#include "PropertyTopoShape.h"
#include <App/GeoFeature.h>
#include <App/FeaturePython.h>
#include <App/PropertyGeo.h>
// includes for findAllFacesCutBy()
#include <TopoDS_Face.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
class gp_Dir;

class BRepBuilderAPI_MakeShape;

namespace Part
{

class PartFeaturePy;

/** Base class of all shape feature classes in FreeCAD
 */
class PartExport Feature : public App::GeoFeature
{
    PROPERTY_HEADER(Part::Feature);

public:
    /// Constructor
    Feature(void);
    virtual ~Feature();

    PropertyPartShape Shape;
    App::PropertyLinkSubHidden ColoredElements;

    /** @name methods override feature */
    //@{
    virtual short mustExecute(void) const;
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const;
    virtual const App::PropertyComplexGeoData* getPropertyOfGeometry() const;

    virtual PyObject* getPyObject(void);

    virtual DocumentObject *getSubObject(const char *subname, PyObject **pyObj, 
            Base::Matrix4D *mat, bool transform, int depth) const override;

    /** Convenience function to extract shape from fully qualified subname 
     *
     * @param obj: the parent object
     *
     * @param subname: dot separated full qualified subname
     *
     * @param needSubElement: whether to ignore the non-object subelement
     * reference inside \c subname
     *
     * @param pmat: used as current transformation on input, and return the
     * accumulated transformation on output
     *
     * @param owner: return the owner of the shape returned
     *
     * @param resolveLink: if true, resolve link(s) of the returned 'owner'
     * by calling its getLinkedObject(true) function
     *
     * @param transform: if true, apply obj's transformation. Set to false
     * if pmat already include obj's transformation matrix.
     */
    static TopoDS_Shape getShape(const App::DocumentObject *obj,
            const char *subname=0, bool needSubElement=false, Base::Matrix4D *pmat=0, 
            App::DocumentObject **owner=0, bool resolveLink=true, bool transform=true);

    static TopoShape getTopoShape(const App::DocumentObject *obj,
            const char *subname=0, bool needSubElement=false, Base::Matrix4D *pmat=0, 
            App::DocumentObject **owner=0, bool resolveLink=true, bool transform=true, 
            bool noElementMap=false);

    static void clearShapeCache();

    struct HistoryItem {
        App::DocumentObject *obj;
        long tag;
        std::string element;
        std::vector<std::string> intermediates;
        HistoryItem(App::DocumentObject *obj, const char *name)
            :obj(obj),tag(0),element(name)
        {
            if(obj)
                tag = obj->getID();
        }
    };
    static std::list<HistoryItem> getElementHistory(App::DocumentObject *obj,
            const char *name, bool recursive=true, bool sameType=false);

    static App::DocumentObject *getShapeOwner(const App::DocumentObject *obj, const char *subname=0);

    static bool hasShapeOwner(const App::DocumentObject *obj, const char *subname=0) {
        auto owner = getShapeOwner(obj,subname);
        return owner && owner->isDerivedFrom(getClassTypeId());
    }

    static std::vector<std::pair<std::string,std::string> > 
    getRelatedElements(App::DocumentObject *obj, const char *name, bool sameType=true, bool withCache=true);

    TopLoc_Location getLocation() const;
    
protected:
    /// recompute only this object
    virtual App::DocumentObjectExecReturn *recompute(void);
    /// recalculate the feature
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onChanged(const App::Property* prop);
};

class FilletBase : public Part::Feature
{
    PROPERTY_HEADER(Part::FilletBase);

public:
    FilletBase();

    App::PropertyLink   Base;
    PropertyFilletEdges Edges;
    App::PropertyLinkSub   EdgeLinks;

    short mustExecute() const;
    virtual void onUpdateElementReference(const App::Property *prop) override;

protected:
    virtual void onDocumentRestored() override;
    virtual void onChanged(const App::Property *) override;
    void syncEdgeLink();
};

typedef App::FeaturePythonT<Feature> FeaturePython;


/** Base class of all shape feature classes in FreeCAD
 */
class PartExport FeatureExt : public Feature
{
    PROPERTY_HEADER(Part::FeatureExt);

public:
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderPartExt";
    }
};

// Utility methods
/**
 * Find all faces cut by a line through the centre of gravity of a given face
 * Useful for the "up to face" options to pocket or pad
 */
struct cutFaces {
    TopoDS_Face face;
    double distsq;
};

PartExport
std::vector<cutFaces> findAllFacesCutBy(const TopoDS_Shape& shape,
                                        const TopoDS_Shape& face, const gp_Dir& dir);

/**
  * Check for intersection between the two shapes. Only solids are guaranteed to work properly
  * There are two modes:
  * 1. Bounding box check only - quick but inaccurate
  * 2. Bounding box check plus (if necessary) boolean operation - costly but accurate
  * Return true if the shapes intersect, false if they don't
  * The flag touch_is_intersection decides whether shapes touching at distance zero are regarded
  * as intersecting or not
  * 1. If set to true, a true check result means that a boolean fuse operation between the two shapes
  *    will return a single solid
  * 2. If set to false, a true check result means that a boolean common operation will return a
  *    valid solid
  * If there is any error in the boolean operations, the check always returns false
  */
PartExport
bool checkIntersection(const TopoDS_Shape& first, const TopoDS_Shape& second,
                       const bool quick, const bool touch_is_intersection);

} //namespace Part


#endif // PART_FEATURE_H

