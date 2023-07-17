#pragma once

#include "EndFrameqc.h"
#include "Symbolic.h"

namespace MbD {
	class Time;

	class EndFrameqct : public EndFrameqc
	{
		//time rmemBlks prmemptBlks pprmemptptBlks phiThePsiBlks pPhiThePsiptBlks ppPhiThePsiptptBlks 
		//rmem prmempt pprmemptpt aAme pAmept ppAmeptpt prOeOpt pprOeOpEpt pprOeOptpt pAOept ppAOepEpt ppAOeptpt 
	public:
		EndFrameqct();
		EndFrameqct(const char* str);
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void initprmemptBlks();
		void initpprmemptptBlks();
		void initpPhiThePsiptBlks();
		void initppPhiThePsiptptBlks();
		void postInput() override;
		void calcPostDynCorrectorIteration() override;
		void prePosIC() override;
		void evalrmem();
		void evalAme();
		void preVelIC() override;
		void postVelIC() override;
		FColDsptr pAjOept(int j);
		FMatDsptr ppAjOepETpt(int j);
		FColDsptr ppAjOeptpt(int j);
		double time = 0.0;
		double priOeOpt(int i);
		FRowDsptr ppriOeOpEpt(int i);
		double ppriOeOptpt(int i);
		void evalprmempt();
		void evalpAmept();
		void evalpprmemptpt();
		void evalppAmeptpt();
		FColDsptr rmeO() override;
		FColDsptr rpep() override;

		void preAccIC() override;

		std::shared_ptr<FullColumn<Symsptr>> rmemBlks, prmemptBlks, pprmemptptBlks;
		std::shared_ptr<FullColumn<Symsptr>> phiThePsiBlks, pPhiThePsiptBlks, ppPhiThePsiptptBlks;
		FColDsptr rmem, prmempt, pprmemptpt, prOeOpt, pprOeOptpt;
		FMatDsptr aAme, pAmept, ppAmeptpt, pAOept, ppAOeptpt;
		FMatDsptr pprOeOpEpt;
		std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> ppAOepEpt;
		
	};
	using EndFrmqctptr = std::shared_ptr<EndFrameqct>;
}

