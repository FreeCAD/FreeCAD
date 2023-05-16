#pragma once
#include "EndFrameqc.h"

namespace MbD {

	class Symbolic;

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

		double time;
		std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> rmemBlks, prmemptBlks, pprmemptptBlks;
		std::shared_ptr<FullColumn<std::shared_ptr<Symbolic>>> phiThePsiBlks, pPhiThePsiptBlks, ppPhiThePsiptptBlks;
		FColDsptr rmem, prmempt, pprmemptpt, pprOeOptpt;
		FMatDsptr aAme, pAmept, ppAmeptpt, ppAOeptpt;
		FMatDsptr pprOeOpEpt;
		std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> ppAOepEpt;
	};
	using EndFrmqctptr = std::shared_ptr<EndFrameqct>;
}

