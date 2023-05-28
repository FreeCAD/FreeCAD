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
		void initialize();
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

		double time = 0.0;
		std::shared_ptr<FullColumn<Symsptr>> rmemBlks, prmemptBlks, pprmemptptBlks;
		std::shared_ptr<FullColumn<Symsptr>> phiThePsiBlks, pPhiThePsiptBlks, ppPhiThePsiptptBlks;
		FColDsptr rmem, prmempt, pprmemptpt, pprOeOptpt;
		FMatDsptr aAme, pAmept, ppAmeptpt, ppAOeptpt;
		FMatDsptr pprOeOpEpt;
		std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> ppAOepEpt;
	};
	using EndFrmqctptr = std::shared_ptr<EndFrameqct>;
}

