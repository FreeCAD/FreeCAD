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

#ifndef ADAPTIVE_HPP
#define ADAPTIVE_HPP

#ifndef __DBL_MAX__
#define __DBL_MAX__ 1.7976931348623158e+308
#endif

#ifndef __LONG_MAX__
#define __LONG_MAX__ 2147483647
#endif

#ifndef M_PI
#define M_PI 3.141592653589793238
#endif

// #define DEV_MODE

#define NTOL 1.0e-7 // numeric tolerance

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
typedef std::pair<int, DPath> TPath; // first parameter is MotionType, must use int due to problem with serialization to JSON in python

class ClearedArea;

typedef std::vector<TPath> TPaths;

struct AdaptiveOutput
{
	DPoint HelixCenterPoint;
	DPoint StartPoint;
	TPaths AdaptivePaths;
	int ReturnMotionType; // MotionType enum, problem with serialization if enum is used
};

// used to isolate state -> enable potential adding of multi-threaded processing of separate regions

class Adaptive2d
{
  public:
	Adaptive2d();
	double toolDiameter = 5;
	double helixRampDiameter = 0;
	double stepOverFactor = 0.2;
	double tolerance = 0.1;
	double stockToLeave = 0;
	bool forceInsideOut = true;
	bool finishingProfile = true;
	double keepToolDownDistRatio = 3.0; // keep tool down distance ratio
	OperationType opType = OperationType::otClearingInside;

	std::list<AdaptiveOutput> Execute(const DPaths &stockPaths, const DPaths &paths, std::function<bool(TPaths)> progressCallbackFn);

#ifdef DEV_MODE
	/*for debugging*/
	std::function<void(double cx, double cy, double radius, int color)> DrawCircleFn;
	std::function<void(const DPath &, int color)> DrawPathFn;
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
	long helixRampRadiusScaled = 0;
	double referenceCutArea = 0;
	double optimalCutAreaPD = 0;
	bool stopProcessing = false;
	int current_region=0;
	clock_t lastProgressTime = 0;

	std::function<bool(TPaths)> *progressCallback = NULL;
	Path toolGeometry; // tool geometry at coord 0,0, should not be modified

	void ProcessPolyNode(Paths boundPaths, Paths toolBoundPaths);
	bool FindEntryPoint(TPaths &progressPaths, const Paths &toolBoundPaths, const Paths &bound, ClearedArea &cleared /*output*/,
						IntPoint &entryPoint /*output*/, IntPoint &toolPos, DoublePoint &toolDir);
	bool FindEntryPointOutside(TPaths &progressPaths, const Paths &toolBoundPaths, const Paths &bound, ClearedArea &cleared /*output*/,
							   IntPoint &entryPoint /*output*/, IntPoint &toolPos, DoublePoint &toolDir);
	double CalcCutArea(Clipper &clip, const IntPoint &toolPos, const IntPoint &newToolPos, ClearedArea &clearedArea, bool preventConventionalMode = true);
	void AppendToolPath(TPaths &progressPaths, AdaptiveOutput &output, const Path &passToolPath, ClearedArea &clearedAreaBefore,
						ClearedArea &clearedAreaAfter, const Paths &toolBoundPaths);
	bool IsClearPath(const Path &path, ClearedArea &clearedArea, double safetyDistanceScaled = 0);
	bool IsAllowedToCutTrough(const IntPoint &p1, const IntPoint &p2, ClearedArea &clearedArea, const Paths &toolBoundPaths, double areaFactor = 1.5, bool skipBoundsCheck = false);
	bool MakeLeadPath(bool leadIn, const IntPoint &startPoint, const DoublePoint &startDir, const IntPoint &beaconPoint,
					  ClearedArea &clearedArea, const Paths &toolBoundPaths, Path &output);

	bool ResolveLinkPath(const IntPoint &startPoint, const IntPoint &endPoint, ClearedArea &clearedArea, Path &output);

	friend class EngagePoint; // for CalcCutArea

	void CheckReportProgress(TPaths &progressPaths, bool force = false);
	void AddPathsToProgress(TPaths &progressPaths, const Paths paths, MotionType mt = MotionType::mtCutting);
	void AddPathToProgress(TPaths &progressPaths, const Path pth, MotionType mt = MotionType::mtCutting);
	void ApplyStockToLeave(Paths &inputPaths);

  private: // constants for fine tuning
	const double RESOLUTION_FACTOR = 16.0;
	const int MAX_ITERATIONS = 10;
	const double AREA_ERROR_FACTOR = 0.05;	/* how precise to match the cut area to optimal, reasonable value: 0.05 = 5%*/
	const size_t ANGLE_HISTORY_POINTS = 3;	// used for angle prediction
	const int DIRECTION_SMOOTHING_BUFLEN = 3; // gyro points - used for angle smoothing


	const double MIN_CUT_AREA_FACTOR = 0.1;// used for filtering out of insignificant cuts (should be < ENGAGE_AREA_THR_FACTOR)
	const double ENGAGE_AREA_THR_FACTOR = 0.5;		// influences minimal engage area
	const double ENGAGE_SCAN_DISTANCE_FACTOR = 0.2; // influences the engage scan/stepping distance

	const double CLEAN_PATH_TOLERANCE = 1.41; // should be >1
	const double FINISHING_CLEAN_PATH_TOLERANCE = 1.41; // should be >1

	const long PASSES_LIMIT = __LONG_MAX__;			   // limit used while debugging
	const long POINTS_PER_PASS_LIMIT = __LONG_MAX__;   // limit used while debugging
	const clock_t PROGRESS_TICKS = CLOCKS_PER_SEC / 10; // progress report interval
};
} // namespace AdaptivePath
#endif