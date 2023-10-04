/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>
#include <functional>

#include "CartesianFrame.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "EulerParametersDot.h"
#include "EulerParametersDDot.h"

namespace MbD {
	class CartesianFrame;
	class PartFrame;
	class EndFramec;
	using EndFrmsptr = std::shared_ptr<EndFramec>;

	class MarkerFrame : public CartesianFrame
	{
		//partFrame rpmp aApm rOmO aAOm prOmOpE pAOmpE pprOmOpEpE ppAOmpEpE endFrames 
	public:
		MarkerFrame();
        MarkerFrame(const char* str);
		System* root() override;
		void initialize() override;
		void setPartFrame(PartFrame* partFrm);
		PartFrame* getPartFrame();
		void setrpmp(FColDsptr x);
		void setaApm(FMatDsptr mat);
		void addEndFrame(EndFrmsptr x);
		void initializeLocally() override;
		void initializeGlobally() override;
		void postInput() override;
		void calcPostDynCorrectorIteration() override;
		void prePosIC() override;
		int iqX();
		int iqE();
		void endFramesDo(const std::function <void(EndFrmsptr)>& f);
		void fillqsu(FColDsptr col) override;
		void fillqsuWeights(DiagMatDsptr diagMat) override;
		void fillqsuddotlam(FColDsptr col) override;
		void fillqsulam(FColDsptr col) override;
		void fillqsudot(FColDsptr col) override;
		void fillqsudotWeights(DiagMatDsptr diagMat) override;
		void setqsu(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		void setqsudot(FColDsptr col) override;
		void setqsudotlam(FColDsptr col) override;
		void setqsuddotlam(FColDsptr col) override;
		void postPosICIteration() override;
		void postPosIC() override;
		void preDyn() override;
		void storeDynState() override;
		void preVelIC() override;
		void postVelIC() override;
		void preAccIC() override;
		FColDsptr qXdot();
		std::shared_ptr<EulerParametersDot<double>> qEdot();
		FColDsptr qXddot();
		FColDsptr qEddot();
		FColFMatDsptr pAOppE();
		FMatDsptr aBOp();
		void postDynStep() override;

		PartFrame* partFrame; //Use raw pointer when pointing backwards.
		FColDsptr rpmp = std::make_shared<FullColumn<double>>(3);
		FMatDsptr aApm = std::make_shared<FullMatrix<double>>(3, 3);
		FColDsptr rOmO = std::make_shared<FullColumn<double>>(3);
		FMatDsptr aAOm = std::make_shared<FullMatrix<double>>(3, 3);
		FMatDsptr prOmOpE;
		FColFMatDsptr pAOmpE;
		FMatFColDsptr pprOmOpEpE;
		FMatFMatDsptr ppAOmpEpE;
		std::shared_ptr<std::vector<EndFrmsptr>> endFrames;

	};
}

