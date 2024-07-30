/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "EndFrameqc.h"
//#include "Symbolic.h"

namespace MbD {
	class Time;
	class Symbolic;

	class EndFrameqct : public EndFrameqc
	{
		//time rmemBlks prmemptBlks pprmemptptBlks phiThePsiBlks pPhiThePsiptBlks ppPhiThePsiptptBlks 
		//rmem prmempt pprmemptpt aAme pAmept ppAmeptpt prOeOpt pprOeOpEpt pprOeOptpt pAOept ppAOepEpt ppAOeptpt 
	public:
		EndFrameqct();
		EndFrameqct(const std::string& str);
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void initprmemptBlks();
		void initpprmemptptBlks();
		virtual void initpPhiThePsiptBlks();
		virtual void initppPhiThePsiptptBlks();
		void postInput() override;
		void calcPostDynCorrectorIteration() override;
		void prePosIC() override;
		void evalrmem();
		virtual void evalAme();
		void preVelIC() override;
		void postVelIC() override;
		FColDsptr pAjOept(size_t j);
		FMatDsptr ppAjOepETpt(size_t j);
		FColDsptr ppAjOeptpt(size_t j);
		double time = 0.0;
		double priOeOpt(size_t i);
		FRowDsptr ppriOeOpEpt(size_t i);
		double ppriOeOptpt(size_t i);
		void evalprmempt();
		virtual void evalpAmept();
		void evalpprmemptpt();
		virtual void evalppAmeptpt();
		FColDsptr rmeO() override;
		FColDsptr rpep() override;
		void preAccIC() override;
		bool isEndFrameqc() override;

		std::shared_ptr<FullColumn<Symsptr>> rmemBlks, prmemptBlks, pprmemptptBlks;
		std::shared_ptr<FullColumn<Symsptr>> phiThePsiBlks, pPhiThePsiptBlks, ppPhiThePsiptptBlks;
		FColDsptr rmem, prmempt, pprmemptpt, prOeOpt, pprOeOptpt;
		FMatDsptr aAme, pAmept, ppAmeptpt, pAOept, ppAOeptpt;
		FMatDsptr pprOeOpEpt;
		FColFMatDsptr ppAOepEpt;
	};
}

