#pragma once
namespace MbD {
	enum ConstraintType { essential, displacement, perpendicular, redundant };
	enum DiscontinuityType { TOUCHDOWN, REBOUND, LIFTOFF };
	enum AnalysisType { INPUT, INITIALCONDITION, DYNAMIC, STATIC };
}