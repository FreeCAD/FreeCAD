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
		void setrpmp(FullColDptr x);
		void setaApm(FullMatDptr x);
		void addEndFrame(std::shared_ptr<EndFramec> x);

		PartFrame* partFrame;
		FullColDptr rpmp = std::make_shared<FullColumn<double>>(3);
		FullMatDptr aApm = std::make_shared<FullMatrix<double>>(3, 3);
		FullColDptr rOmO = std::make_shared<FullColumn<double>>(3);
		FullMatDptr aAOm = std::make_shared<FullMatrix<double>>(3, 3);
		FullMatDptr prOmOpE = std::make_shared<FullMatrix<double>>(3, 4);
		FullColumn<FullMatrix<double>>* pAOmpE = new FullColumn<FullMatrix<double>>(4);
		std::unique_ptr<std::vector<std::shared_ptr<EndFramec>>> endFrames;

	};
}

