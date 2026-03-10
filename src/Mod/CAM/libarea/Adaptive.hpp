// SPDX-License-Identifier: LGPL-2.1-or-later

/**************************************************************************
 *   Copyright (c) 2018 Kresimir Tusek <kresimir.tusek@gmail.com>          *
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

#include "clipper.hpp"
#include <vector>
#include <list>
#include <time.h>

#pragma once

#ifndef __DBL_MAX__
# define __DBL_MAX__ 1.7976931348623158e+308
#endif

#ifndef __LONG_MAX__
# define __LONG_MAX__ 2147483647
#endif

// #define DEV_MODE

#define NTOL 1.0e-7  // numeric tolerance

namespace AdaptivePath
{
using namespace ClipperLib;

enum MotionType
{
    mtCutting = 0,
    mtLinkClear = 1,
    mtLinkNotClear = 2,
    mtLinkClearAtPrevPass = 3
};

enum OperationType
{
    otClearingInside = 0,
    otClearingOutside = 1,
    otProfilingInside = 2,
    otProfilingOutside = 3
};

typedef std::pair<double, double> DPoint;
typedef std::vector<DPoint> DPath;
typedef std::vector<DPath> DPaths;
typedef std::pair<int, DPath> TPath;  // first parameter is MotionType, must use int due to problem
                                      // with serialization to JSON in python

class ClearedArea;

typedef std::vector<TPath> TPaths;

struct AdaptiveOutput
{
    DPoint HelixCenterPoint;
    DPoint StartPoint;
    TPaths AdaptivePaths;
    int ReturnMotionType;  // MotionType enum, problem with serialization if enum is used
};

// used to isolate state -> enable potential adding of multi-threaded processing of separate regions

class Adaptive2d
{
public:
    Adaptive2d();
    double toolDiameter = 5;
    double helixRampTargetDiameter = 0;
    double helixRampMinDiameter = 0;
    double stepOverFactor = 0.2;
    double tolerance = 0.1;
    double stockToLeave = 0;
    bool forceInsideOut = true;
    bool finishingProfile = true;
    double keepToolDownDistRatio = 3.0;  // keep tool down distance ratio
    OperationType opType = OperationType::otClearingInside;

    std::list<AdaptiveOutput> Execute(
        const DPaths& stockPaths,
        const DPaths& paths,
        std::function<bool(TPaths)> progressCallbackFn
    );

#ifdef DEV_MODE
    /*for debugging*/
    std::function<void(double cx, double cy, double radius, int color)> DrawCircleFn;
    std::function<void(const DPath&, int color)> DrawPathFn;
    std::function<void()> ClearScreenFn;
#endif

private:
    std::list<AdaptiveOutput> results;
    Paths inputPaths;
    Paths stockInputPaths;
    int polyTreeNestingLimit = 0;
    long scaleFactor = 100;
    double stepOverScaled = 1;
    long toolRadiusScaled = 10;
    long finishPassOffsetScaled = 0;
    long helixRampMaxRadiusScaled = 0;
    long helixRampMinRadiusScaled = 0;
    double referenceCutArea = 0;
    double optimalCutAreaPD = 0;
    bool stopProcessing = false;
    int current_region = 0;
    clock_t lastProgressTime = 0;

    std::function<bool(TPaths)>* progressCallback = NULL;
    Path toolGeometry;  // tool geometry at coord 0,0, should not be modified

    void ProcessPolyNode(Paths boundPaths, Paths toolBoundPaths);
    bool FindEntryPoint(
        TPaths& progressPaths,
        const Paths& toolBoundPaths,
        const Paths& bound,
        ClearedArea& cleared /*output*/,
        IntPoint& entryPoint /*output*/,
        IntPoint& toolPos,
        DoublePoint& toolDir,
        long& helixRadiusScaled
    );
    bool FindEntryPointOutside(
        TPaths& progressPaths,
        const Paths& toolBoundPaths,
        const Paths& bound,
        ClearedArea& cleared /*output*/,
        IntPoint& entryPoint /*output*/,
        IntPoint& toolPos,
        DoublePoint& toolDir
    );
    double CalcCutArea(
        Clipper& clip,
        const IntPoint& toolPos,
        const IntPoint& newToolPos,
        ClearedArea& clearedArea,
        bool preventConventionalMode = true
    );
    void AppendToolPath(
        TPaths& progressPaths,
        AdaptiveOutput& output,
        const Path& passToolPath,
        ClearedArea& clearedAreaBefore,
        ClearedArea& clearedAreaAfter,
        const Paths& toolBoundPaths
    );
    bool IsClearPath(const Path& path, ClearedArea& clearedArea, double safetyDistanceScaled = 0);
    bool IsAllowedToCutTrough(
        const IntPoint& p1,
        const IntPoint& p2,
        ClearedArea& clearedArea,
        const Paths& toolBoundPaths,
        double areaFactor = 1.5,
        bool skipBoundsCheck = false
    );
    bool MakeLeadPath(
        bool leadIn,
        const IntPoint& startPoint,
        const DoublePoint& startDir,
        const IntPoint& beaconPoint,
        ClearedArea& clearedArea,
        const Paths& toolBoundPaths,
        Path& output
    );

    bool ResolveLinkPath(
        const IntPoint& startPoint,
        const IntPoint& endPoint,
        ClearedArea& clearedArea,
        Path& output
    );

    friend class EngagePoint;  // for CalcCutArea

    void CheckReportProgress(TPaths& progressPaths, bool force = false);
    void AddPathsToProgress(
        TPaths& progressPaths,
        const Paths paths,
        MotionType mt = MotionType::mtCutting
    );
    void AddPathToProgress(TPaths& progressPaths, const Path pth, MotionType mt = MotionType::mtCutting);
    void ApplyStockToLeave(Paths& inputPaths);

private:
    // Derivation for MIN_STEP_CLIPPPER (MSC for short in this derivation):
    // Diagram:
    // - circle C1 from previous pass, radius R
    // - circle C2 from current pass MSC away, horizontal
    // - line Lprev from previous pass, step over x
    // - line Llong from tool position through C1/Lprev intersection to C2
    // - line L1 from previous tool position to C1/Lprev intersection
    //
    // Length of Llong = R + y, where y is the longest protrusion into the cut area
    // When selecting MIN_STEP_CLIPPER, we need to ensure that the computed
    // value for y > 1 when using stepover x equal to the size of the finishing
    // pass. Finishing pass stepover is
    // x = stepover/10
    // x = 2 * R * stepoverFactor / 10 (Eq1).
    //
    // Construct right triangle with (R-x) of vertical radius from C1 and
    // L1. Third length (horizontal) = a
    // (R-x)^2 + a^2 = R^2
    // a^2 = 2*R*x - x^2  (Eq2)
    // a ~= sqrt(2*R*x)  (Eq3; x<<R)
    //
    // Construct right traingle with (R-x) of vertical radius from C2 and (R-y)
    // of Llong. Third length is MSC - a (horizontal).
    // (MSC-a)^2 + (R-x)^2 = (R-y)^2
    // MSC^2 - 2*a*MSC + a^2 + R^2 - 2*R*x + x^2 = R^2 - 2*R*y + y^2
    // MSC^2 - 2*a*MSC + a^2 - 2*R*x + x^2 = -2*R*y + y^2
    // MSC^2 - 2*MSC*sqrt(2*R*x) + (2*R*x - x^2) - 2*R*x + x^2 = - 2*R*y + y^2 (substitute Eq2, Eq3)
    // MSC^2 - 2*MSC*sqrt(2*R*x) = - 2*R*y + y^2
    // MSC^2 - 2*MSC*sqrt(2*R*(2*R*stepoverFactor/10)) = -2*R*y + y^2 (substitute Eq1)
    // MSC^2 - 2*R*MSC*sqrt(2*stepoverFactor/5) = -2*R*y + y^2
    // -2*R*MSC*sqrt(2*stepoverFactor/5) = -2*R*y (MSC << R, y << R)
    // MSC*sqrt(2*stepoverFactor/5) = y
    // MSC = y/sqrt(2*stepoverFactor/5)   (Eq4)
    //
    // To ensure we don't evaluate a postive cut area as zero, we need y to
    // measure > 1. The endpoints of y may be perturbed by up to sqrt(2)/2 each
    // due to integer rounding, so the true value of y must be at least 1+sqrt(2) ~= 2.4.
    // StepoverFactor may be as small as 1% = 0.01. Evaluating Eq4 with these values:
    //
    // MSC > 2.4/sqrt(2*.01/5)
    // MSC > 38.
    //
    // Historically we have used MSC = 16. It might be convenient that MSC is
    // many-times divisible by 2, so I have chosen 16*3 (>38) for its new value.
    const double MIN_STEP_CLIPPER = 16.0 * 3;
    const int MAX_ITERATIONS = 10;
    const double AREA_ERROR_FACTOR = 0.05;     /* how precise to match the cut area to optimal,
                                                  reasonable value: 0.05 = 5%*/
    const size_t ANGLE_HISTORY_POINTS = 3;     // used for angle prediction
    const int DIRECTION_SMOOTHING_BUFLEN = 3;  // gyro points - used for angle smoothing


    const double MIN_CUT_AREA_FACTOR = 0.1;          // used for filtering out of insignificant cuts
    const double ENGAGE_AREA_THR_FACTOR = .3;        // influences minimal engage area
    const double ENGAGE_SCAN_DISTANCE_FACTOR = 0.2;  // influences the engage scan/stepping distance

    const double CLEAN_PATH_TOLERANCE = 1.41;            // should be >1
    const double FINISHING_CLEAN_PATH_TOLERANCE = 1.41;  // should be >1

    const long PASSES_LIMIT = __LONG_MAX__;              // limit used while debugging
    const long POINTS_PER_PASS_LIMIT = __LONG_MAX__;     // limit used while debugging
    const clock_t PROGRESS_TICKS = CLOCKS_PER_SEC / 10;  // progress report interval
};
}  // namespace AdaptivePath
