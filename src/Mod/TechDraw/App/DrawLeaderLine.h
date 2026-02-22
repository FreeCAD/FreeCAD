/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "DrawView.h"


namespace TechDraw
{
class DrawViewPart;

class TechDrawExport DrawLeaderLine : public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawLeaderLine);

public:
    DrawLeaderLine();
    ~DrawLeaderLine() override = default;

    App::PropertyLink         LeaderParent;
    App::PropertyVectorList   WayPoints;
    App::PropertyEnumeration  StartSymbol;
    App::PropertyEnumeration  EndSymbol;

    App::PropertyBool         Scalable;
    App::PropertyBool         AutoHorizontal;
    App::PropertyBool         RotatesWithParent;

    short mustExecute() const override;
    App::DocumentObjectExecReturn *execute() override;
    App::PropertyLink *getOwnerProperty() override { return &LeaderParent; }

    const char* getViewProviderName() const override {
        return "TechDrawGui::ViewProviderLeader";
    }
    PyObject *getPyObject() override;
    QRectF getRect() const override { return { 0, 0,1, 1}; }

    Base::Vector3d getAttachPoint();
    DrawView* getBaseView() const;
    virtual App::DocumentObject* getBaseObject() const;

    bool keepUpdated() override;
    double getScale() const override;
    double getBaseScale() const;
    static std::vector<Base::Vector3d> horizLastSegment(const std::vector<Base::Vector3d>& inDeltas, double rotationDeg);
    bool getDefAuto() const;

    Base::Vector3d getTileOrigin() const;
    Base::Vector3d getKinkPoint() const;
    Base::Vector3d getTailPoint() const;

    static DrawLeaderLine* makeLeader(DrawViewPart* parent, std::vector<Base::Vector3d> points, int iStartSymbol = 0, int iEndSymbol = 0);
    std::vector<Base::Vector3d>  getScaledAndRotatedPoints(bool doScale = true, bool doRotate = true) const;
    std::vector<Base::Vector3d> makeCanonicalPoints(const std::vector<Base::Vector3d>& inPoints,
                                                    bool doScale = true,
                                                    bool doRotate = true) const;
    std::vector<Base::Vector3d> makeCanonicalPointsInverted(const std::vector<Base::Vector3d>& inPoints,
                                                    bool doScale = true,
                                                    bool doRotate = true) const;

    bool isParentReady() const;

    void dumpWaypoints(const std::vector<Base::Vector3d>& points, const std::string& label);

    std::vector<Base::Vector3d> getTransformedWayPoints() const;

    Base::Vector3d lastSegmentDirection() const;

    bool snapsToPosition() const override { return false; }

private:


};

using DrawLeaderLinePython = App::FeaturePythonT<DrawLeaderLine>;

} //namespace TechDraw