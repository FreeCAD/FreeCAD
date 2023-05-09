#pragma once
#include "EndFrameqc.h"
#include "Variable.h"

namespace MbD {
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

		double time;
		std::shared_ptr<FullColumn<std::shared_ptr<Variable>>> rmemBlks;
		std::shared_ptr<FullColumn<std::shared_ptr<Variable>>> phiThePsiBlks;
		FColDuptr rmem, prmempt, pprmemptpt, pprOeOptpt;
		FMatDuptr aAme, pAmept, ppAmeptpt, ppAOeptpt;
		FMatDuptr pprOeOpEpt;
		std::unique_ptr<FullColumn<FullMatrix<double>>> ppAOepEpt;
	};
}

