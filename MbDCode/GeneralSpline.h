/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "AnyGeneralSpline.h"

namespace MbD {
	class GeneralSpline : public AnyGeneralSpline
	{
		//derivs degree index delta 
	public:
		GeneralSpline() = default;
		GeneralSpline(Symsptr arg);
		double getValue() override;
		Symsptr differentiateWRTx() override;
		void arguments(Symsptr args) override;
		void initxdegreexsys(Symsptr arg, int order, std::shared_ptr<std::vector<double>> xarr, std::shared_ptr<std::vector<double>> yarr);
		void computeDerivatives();
		bool isCyclic();
		double derivativeAt(int derivativeOrder, double arg);
		void calcIndexAndDeltaFor(double xxx);
		void calcCyclicIndexAndDelta();
		void calcNonCyclicIndexAndDelta();
		void calcIndexAndDelta();
		void searchIndexFromto(int start, int end);
		Symsptr clonesptr() override;
		double y(double xxx);

		std::ostream& printOn(std::ostream& s) const override;

		FMatDsptr derivs;
		int degree, index;
		double delta;
	};
}

