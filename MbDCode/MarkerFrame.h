#pragma once
#include <memory>

#include "CartesianFrame.h"
#include "PartFrame.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "EndFramec.h"

namespace MbD {
	class PartFrame;
	class EndFramec;

	class MarkerFrame : public CartesianFrame
	{
		//partFrame rpmp aApm rOmO aAOm prOmOpE pAOmpE pprOmOpEpE ppAOmpEpE endFrames 
	public:
		MarkerFrame();
        MarkerFrame(const char* str);
        void initialize();
		void setPartFrame(PartFrame* partFrm);
		PartFrame* getPartFrame();
		void setrpmp(FColDsptr x);
		void setaApm(FMatDsptr x);
		void addEndFrame(std::shared_ptr<EndFramec> x);
		void initializeLocally() override;
		void initializeGlobally() override;

		PartFrame* partFrame;
		FColDsptr rpmp = std::make_shared<FullColumn<double>>(3);
		FMatDsptr aApm = std::make_shared<FullMatrix<double>>(3, 3);
		FColDsptr rOmO = std::make_shared<FullColumn<double>>(3);
		FMatDsptr aAOm = std::make_shared<FullMatrix<double>>(3, 3);
		FMatDsptr prOmOpE;
		std::unique_ptr<FullColumn<FullMatrix<double>>> pAOmpE;
		std::unique_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> pprOmOpEpE;
		FMatFMatDuptr ppAOmpEpE;
		std::unique_ptr<std::vector<std::shared_ptr<EndFramec>>> endFrames;

	};
}

