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


//#define DEV_MODE

#define NTOL 1.0e-7  // numeric tolerance

namespace AdaptivePath {
	using namespace ClipperLib;

	enum MotionType { mtCutting = 0, mtLinkClear = 1, mtLinkNotClear = 2, mtLinkClearAtPrevPass = 3  };

	enum OperationType { otClearing = 0, otProfilingInside = 1, otProfilingOutside = 2 };

	typedef std::pair<double,double> DPoint;
	typedef std::vector<DPoint> DPath;
	typedef std::vector<DPath> DPaths;
	typedef std::pair<int,DPath> TPath; // first parameter is MotionType, must use int due to problem with serialization to JSON in python

	// struct TPath { #this does not work correctly with pybind, changed to pair
	// 		DPath Points;
	// 		MotionType MType;
	// };

	typedef std::vector<TPath> TPaths;

	struct AdaptiveOutput {
		DPoint HelixCenterPoint;
		DPoint StartPoint;
		TPaths AdaptivePaths;
		int ReturnMotionType; // MotionType enum, problem with serialization if enum is used
	};

	// used to isolate state -> enable potential adding of multi-threaded processing of separate regions

	class Adaptive2d {
		public:
			Adaptive2d();
			double toolDiameter=5;
			double helixRampDiameter=0;
			double stepOverFactor = 0.2;
			int polyTreeNestingLimit=0;
			double tolerance=0.1;
			double stockToLeave=0;
			OperationType opType = OperationType::otClearing;

			std::list<AdaptiveOutput> Execute(const DPaths &paths, std::function<bool(TPaths)> progressCallbackFn);

			#ifdef DEV_MODE
			/*for debugging*/
			std::function<void(double cx,double cy, double radius, int color)> DrawCircleFn;
			std::function<void(const DPath &, int color)> DrawPathFn;
			std::function<void()> ClearScreenFn;
			#endif

		private:
			std::list<AdaptiveOutput> results;
			Paths inputPaths;

			double scaleFactor=100;
			long toolRadiusScaled=10;
			long finishPassOffsetScaled=0;
			long helixRampRadiusScaled=0;
			long bbox_size=0;
			double referenceCutArea=0;
			double optimalCutAreaPD=0;
			double minCutAreaPD=0;
			bool stopProcessing=false;
			long unclearLinkingMoveCount = 0;
			time_t lastProgressTime = 0;
			
			std::function<bool(TPaths)> * progressCallback=NULL;
			Path toolGeometry; // tool geometry at coord 0,0, should not be modified

			void ProcessPolyNode(Paths & boundPaths, Paths & toolBoundPaths);
			bool FindEntryPoint(const Paths & toolBoundPaths,const Paths &bound, Paths &cleared /*output*/, IntPoint &entryPoint /*output*/);
			double CalcCutArea(Clipper & clip,const IntPoint &toolPos, const IntPoint &newToolPos, const Paths &cleared_paths);
			void AppendToolPath(AdaptiveOutput & output,const Path & passToolPath,const Paths & cleared,const Paths & toolBoundPaths, bool close=false);
			bool  CheckCollision(const IntPoint &lastPoint,const IntPoint &nextPoint,const Paths & cleared);
			friend class EngagePoint; // for CalcCutArea

			void CheckReportProgress(TPaths &progressPaths,bool force=false);

		private: // constants for fine tuning
			const bool preventConvetionalMode = true;
			const double RESOLUTION_FACTOR = 8.0;
			const int MAX_ITERATIONS = 16;
			const double AREA_ERROR_FACTOR = 0.05; /* how precise to match the cut area to optimal, reasonable value: 0.05 = 5%*/
			const size_t ANGLE_HISTORY_POINTS=3; // used for angle prediction
			const int DIRECTION_SMOOTHING_BUFLEN=3; // gyro points - used for angle smoothing

			const double ENGAGE_AREA_THR_FACTOR=0.2; // influences minimal engage area (factor relation to optimal)
			const double ENGAGE_SCAN_DISTANCE_FACTOR=0.1; // influences the engage scan/stepping distance

			const double CLEAN_PATH_TOLERANCE = 0.5;
			const double FINISHING_CLEAN_PATH_TOLERANCE = 0.1;

			// used for filtering out of insignificant cuts:
			const double MIN_CUT_AREA_FACTOR = 0.02; // influences filtering of cuts that with cumulative area below threshold, reasonable value is between 0.01 and 0.1

			const long PASSES_LIMIT = __LONG_MAX__; // limit used while debugging
			const long POINTS_PER_PASS_LIMIT =  __LONG_MAX__; // limit used while debugging
			const time_t PROGRESS_TICKS = CLOCKS_PER_SEC/20; // progress report interval
	};
}
#endif