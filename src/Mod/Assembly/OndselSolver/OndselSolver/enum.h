/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once
namespace MbD {
	enum ConstraintType { essential, displacement, perpendicular, redundant };
	enum DiscontinuityType { TOUCHDOWN, REBOUND, LIFTOFF };
	enum AnalysisType { INPUT, INITIALCONDITION, DYNAMIC, STATIC };
}