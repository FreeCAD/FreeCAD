#pragma once
#include <memory>

#include "CartesianFrame.h"
//#include "PartFrame.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "EndFramec.h"

namespace MbD {
	class PartFrame;
	//class EndFramec;
	//using EndFrmcptr = std::shared_ptr<EndFramec>;

	class MarkerFrame : public CartesianFrame
	{
		//partFrame rpmp aApm rOmO aAOm prOmOpE pAOmpE pprOmOpEpE ppAOmpEpE endFrames 
	public:
		static std::shared_ptr<MarkerFrame> Create(const char* name);
		MarkerFrame();
        MarkerFrame(const char* str);
        void initialize();
		void setPartFrame(PartFrame* partFrm);
		PartFrame* getPartFrame();
		void setrpmp(FColDsptr x);
		void setaApm(FMatDsptr x);
		void addEndFrame(EndFrmcptr x);
		void initializeLocally() override;
		void initializeGlobally() override;
		void postInput() override;
		void calcPostDynCorrectorIteration() override;

		PartFrame* partFrame;
		FColDsptr rpmp = std::make_shared<FullColumn<double>>(3);
		FMatDsptr aApm = std::make_shared<FullMatrix<double>>(3, 3);
		FColDsptr rOmO = std::make_shared<FullColumn<double>>(3);
		FMatDsptr aAOm = std::make_shared<FullMatrix<double>>(3, 3);
		FMatDsptr prOmOpE;
		std::shared_ptr<FullColumn<std::shared_ptr<FullMatrix<double>>>> pAOmpE;
		std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> pprOmOpEpE;
		FMatFMatDsptr ppAOmpEpE;
		std::shared_ptr<std::vector<EndFrmcptr>> endFrames;

	};
}

