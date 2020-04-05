/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "PreCompiled.h"

#include "SketchSolver.h"

using namespace Sketcher;

TYPESYSTEM_SOURCE_ABSTRACT(Sketcher::SketchSolver, Base::Persistence)


void SketchSolver::getSolvableGeometryContraints(const std::vector<Part::Geometry *> & GeoList,
                        const std::vector<Constraint *> & ConstraintList,
                        int extGeoCount,
                        std::vector<Part::Geometry *> & intGeoList, 
                        std::vector<Part::Geometry *> & extGeoList,
                        std::vector<bool> & blockedGeometry,
                        std::vector<bool> & unenforceableConstraints) const
{
    intGeoList.reserve(int(GeoList.size())-extGeoCount);
    extGeoList.reserve(extGeoCount);
    blockedGeometry.assign(int(GeoList.size())-extGeoCount,false);
    unenforceableConstraints.assign(ConstraintList.size(),false);
    
    
    for (int i=0; i < int(GeoList.size())-extGeoCount; i++)
        intGeoList.push_back(GeoList[i]);
    
    for (int i=int(GeoList.size())-extGeoCount; i < int(GeoList.size()); i++)
        extGeoList.push_back(GeoList[i]);

    if(!intGeoList.empty())
        getBlockedGeometry(blockedGeometry, unenforceableConstraints, ConstraintList);
}


void SketchSolver::getBlockedGeometry(std::vector<bool> & blockedGeometry,
                                std::vector<bool> & unenforceableConstraints,
                                const std::vector<Constraint *> &ConstraintList) const
{
    std::vector<int> internalAlignmentConstraintIndex;
    std::vector<int> internalAlignmentgeo;

    std::vector<int> geo2blockingconstraintindex(blockedGeometry.size(),-1);

    // Detect Blocked and internal constraints
    int i = 0;
    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it,++i) {
        switch((*it)->Type) {
            case Block:
            {
                int geoid = (*it)->First;

                if(geoid>=0 && geoid<int(blockedGeometry.size())) {
                    blockedGeometry[geoid]=true;
                    geo2blockingconstraintindex[geoid]=i;
                }
            }
            break;
            case InternalAlignment:
                internalAlignmentConstraintIndex.push_back(i);
            break;
            default:
            break;
        }
    }

    // if a GeoId is blocked and it is linked to Internal Alignment, then GeoIds linked via Internal Alignment are also to be blocked
    for(std::vector<int>::iterator it = internalAlignmentConstraintIndex.begin(); it != internalAlignmentConstraintIndex.end() ; it++) {
        if (blockedGeometry[ConstraintList[(*it)]->Second]) {
            blockedGeometry[ConstraintList[(*it)]->First] = true;
            // associated geometry gets the same blocking constraint index as the blocked element
            geo2blockingconstraintindex[ConstraintList[(*it)]->First]= geo2blockingconstraintindex[ConstraintList[(*it)]->Second];
            internalAlignmentgeo.push_back(ConstraintList[(*it)]->First);
            unenforceableConstraints[(*it)]= true;
        }
    }

    i = 0;
    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it,++i) {
        if((*it)->isDriving) {
            // additionally any further constraint on auxiliary elements linked via Internal Alignment are also unenforceable.
            for(std::vector<int>::iterator itg = internalAlignmentgeo.begin(); itg != internalAlignmentgeo.end() ; itg++) {
                if( (*it)->First==*itg || (*it)->Second==*itg || (*it)->Third==*itg ) {
                    unenforceableConstraints[i]= true;
                }
            }
            // IMPORTANT NOTE:
            // The rest of the ignoring of redundant/conflicting applies to constraints introduced before the blocking constraint only
            // Constraints introduced after the block will not be ignored and will lead to redundancy/conflicting status as per normal
            // solver behaviour

            // further, any constraint taking only one element, which is blocked is also unenforceable
            if((*it)->Second==Constraint::GeoUndef && (*it)->Third==Constraint::GeoUndef && (*it)->First>=0 ) {
                if (blockedGeometry[(*it)->First] && i < geo2blockingconstraintindex[(*it)->First]) {
                    unenforceableConstraints[i]= true;
                }
            }
            // further any constraint on only two elements where both elements are blocked or one is blocked and the other is an axis or external
            // provided that the constraints precede the last block constraint.
            else if((*it)->Third==Constraint::GeoUndef) {
                if ( ((*it)->First>=0 && (*it)->Second>=0 && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second] &&
                    (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Second])) ||
                    ((*it)->First<0 && (*it)->Second>=0 && blockedGeometry[(*it)->Second] && i < geo2blockingconstraintindex[(*it)->Second]) ||
                    ((*it)->First>=0 && (*it)->Second<0 && blockedGeometry[(*it)->First] && i < geo2blockingconstraintindex[(*it)->First]) ){
                    unenforceableConstraints[i]= true;
                }
            }
            // further any constraint on three elements where the three of them are blocked, or two are blocked and the other is an axis or external geo
            // or any constraint on three elements where one is blocked and the other two are axis or external geo, provided that the constraints precede
            // the last block constraint.
            else {
                if( ((*it)->First>=0 && (*it)->Second>=0 && (*it)->Third>=0 &&
                    blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second] && blockedGeometry[(*it)->Third] &&
                  (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Second] || i < geo2blockingconstraintindex[(*it)->Third])) ||
                  ((*it)->First<0 && (*it)->Second>=0 && (*it)->Third>=0 && blockedGeometry[(*it)->Second] && blockedGeometry[(*it)->Third] &&
                  (i < geo2blockingconstraintindex[(*it)->Second] || i < geo2blockingconstraintindex[(*it)->Third])) ||
                  ((*it)->First>=0 && (*it)->Second<0 && (*it)->Third>=0 && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Third] &&
                  (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Third])) ||
                  ((*it)->First>=0 && (*it)->Second>=0 && (*it)->Third<0 && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second] &&
                  (i < geo2blockingconstraintindex[(*it)->First] || i < geo2blockingconstraintindex[(*it)->Second])) ||
                  ((*it)->First>=0 && (*it)->Second<0 && (*it)->Third<0 && blockedGeometry[(*it)->First] && i < geo2blockingconstraintindex[(*it)->First]) ||
                  ((*it)->First<0 && (*it)->Second>=0 && (*it)->Third<0 && blockedGeometry[(*it)->Second] && i < geo2blockingconstraintindex[(*it)->Second]) ||
                  ((*it)->First<0 && (*it)->Second<0 && (*it)->Third>=0 && blockedGeometry[(*it)->Third] && i < geo2blockingconstraintindex[(*it)->Third]) ) {

                    unenforceableConstraints[i]= true;
                }
            }
        }
    }
}
