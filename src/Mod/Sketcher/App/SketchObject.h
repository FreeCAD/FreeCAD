/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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



#ifndef SKETCHER_SKETCHOBJECT_H
#define SKETCHER_SKETCHOBJECT_H

#include <App/PropertyStandard.h>
#include <App/PropertyFile.h>
#include <App/FeaturePython.h>

#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/PropertyGeometryList.h>
#include <Mod/Sketcher/App/PropertyConstraintList.h>

namespace Sketcher
{

class SketcherExport SketchObject : public Part::Part2DObject
{
    PROPERTY_HEADER(Sketcher::SketchObject);

public:
    SketchObject();

    /// Property
    Sketcher::PropertyConstraintList Constraints;
    App     ::PropertyLinkSubList    ExternalConstraints;
    Part    ::PropertyGeometryList   Geometry;
    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "SketcherGui::ViewProviderSketch";
    }
    //@}

    /// add unspecified geometry
    int addGeometry(const Part::Geometry *geo);
    /// add unspecified geometry
    int addGeometry(const std::vector<Part::Geometry *> &geoList);
    /// delete geometry
    int delGeometry(int GeoNbr);
    /// add all constraints in the list
    int addConstraints(const std::vector<Constraint *> &ConstraintList);
    /// add constraint
    int addConstraint(const Constraint *constraint);
    /// delete constraint
    int delConstraint(int ConstrId);
    int delConstraintOnPoint(int GeoId, PointPos PosId, bool onlyCoincident=true);
    int delConstraintOnPoint(int VertexId, bool onlyCoincident=true);
    /// transfers all contraints of a point to a new point
    int transferConstraints(int fromGeoId, PointPos fromPosId, int toGeoId, PointPos toPosId);
    /// add an external geometry reference
    int addExternal(App::DocumentObject *Obj, const char* SubName);
    /// returns a list of projected external geoms
    std::vector<Part::Geometry *> getExternalGeometry(void);
    /// delete external
    int delExternal(int ConstrId);

    /// returns non zero if the sketch contains conflicting constraints 
    int hasConflicts(void) const;

    /// set the datum of a Distance or Angle constraint and solve 
    int setDatum(int ConstrId, double Datum);
    /// move this point to a new location and solve
    int movePoint(int geoIndex1, PointPos Pos1, const Base::Vector3d& toPoint, bool relative=false);
    /// retrieves the coordinates of a point
    Base::Vector3d getPoint(int geoIndex1, PointPos Pos1);

    /// toggle geometry to draft line
    int toggleConstruction(int GeoNbr);

    /// create a fillet
    int fillet(int geoId, PointPos pos, double radius, bool trim=true);
    int fillet(int geoId1, int geoId2,
               const Base::Vector3d& refPnt1, const Base::Vector3d& refPnt2,
               double radius, bool trim=true);

    /// trim a curve
    int trim(int geoId, const Base::Vector3d& point);

    /// retrieves for a Vertex number the corresponding GeoId and PosId
    void getGeoVertexIndex(int VertexId, int &GeoId, PointPos &PosId);
    int getHighestVertexIndex(void) { return VertexId2GeoId.size() - 1; }
    int getHighestCurveIndex(void) { return Geometry.getSize() - 1; }
    void rebuildVertexIndex(void);

    /// retrieves for a Vertex number a list with all coincident points
    void getCoincidentPoints(int GeoId, PointPos PosId, std::vector<int> &GeoIdList,
                             std::vector<PointPos> &PosIdList);
    void getCoincidentPoints(int VertexId, std::vector<int> &GeoIdList, std::vector<PointPos> &PosIdList);

    /// generates a warning message about constraint conflicts and appends it to the given message
    static void appendConflictMsg(const std::vector<int> &conflicting, std::string &msg);

    // from base class
    virtual PyObject *getPyObject(void);
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

private:
    std::vector<int> VertexId2GeoId;
    std::vector<PointPos> VertexId2PosId;
};

typedef App::FeaturePythonT<SketchObject> SketchObjectPython;

} //namespace Sketcher


#endif // SKETCHER_SKETCHOBJECT_H
