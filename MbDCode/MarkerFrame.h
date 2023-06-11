#pragma once
#include <memory>
#include <functional>

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
		void prePosIC() override;
		int iqX();
		int iqE();
		void endFramesDo(const std::function <void(std::shared_ptr<EndFramec>)>& f);
		void fillqsu(FColDsptr col) override;
		void fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat) override;
		void fillqsulam(FColDsptr col) override;
		void setqsu(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		void postPosICIteration() override;
		void postPosIC() override;
		void preDyn() override;

		PartFrame* partFrame; //Use raw pointer when pointing backwards.
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

