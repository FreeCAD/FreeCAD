// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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
// a class to validate and correct dimension references

#ifndef DIMAUTOCORRECT_H
#define DIMAUTOCORRECT_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <DrawViewDimension.h>

namespace Part
{
class TopoShape;
}

namespace TechDraw
{
class GeometryMatcher;


class TechDrawExport DimensionAutoCorrect
{
public:
    DimensionAutoCorrect()
    {}
    explicit DimensionAutoCorrect(DrawViewDimension* dim)
    {
        m_dimension = dim;
    }
    ~DimensionAutoCorrect() = default;

    bool referencesHaveValidGeometry(std::vector<bool>& referenceState) const;
    bool autocorrectReferences(std::vector<bool>& referenceState,
                               ReferenceVector& repairedRefs) const;

    void set3dObjectCache(std::set<std::string> cache)
    {
        m_3dObjectCache = cache;
    }
    bool fixBrokenReferences(ReferenceVector& fixedReferences) const;

private:
    bool fix1GeomExact(ReferenceEntry& refToFix, TopoDS_Shape geomToFix) const;
    bool fix1GeomSimilar(ReferenceEntry& refToFix, TopoDS_Shape geomToFix) const;

    bool findExactVertex2d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;
    bool findExactEdge2d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;
    bool findExactVertex3d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;
    bool findExactEdge3d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;

    bool findSimilarVertex2d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;
    bool findSimilarEdge2d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;
    bool findSimilarVertex3d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;
    bool findSimilarEdge3d(ReferenceEntry& refToFix, Part::TopoShape refGeom) const;

    ReferenceEntry
    searchObjForVert(App::DocumentObject* obj, Part::TopoShape refVertex, bool exact = true) const;
    ReferenceEntry
    searchViewForVert(DrawViewPart* obj, Part::TopoShape refVertex, bool exact = true) const;
    ReferenceEntry
    searchObjForEdge(App::DocumentObject* obj, Part::TopoShape refEdge, bool exact = true) const;

    ReferenceEntry searchViewForExactEdge(DrawViewPart* obj, Part::TopoShape refEdge) const;
    ReferenceEntry searchViewForSimilarEdge(DrawViewPart* obj, Part::TopoShape refEdge) const;


    bool isMatchingGeometry(ReferenceEntry ref, Part::TopoShape savedGeometry) const;

    DrawViewDimension* getDimension() const
    {
        return m_dimension;
    }
    GeometryMatcher* getMatcher() const;

    DrawViewDimension* m_dimension;
    std::set<std::string> m_3dObjectCache;
};

}  // end namespace TechDraw
#endif
