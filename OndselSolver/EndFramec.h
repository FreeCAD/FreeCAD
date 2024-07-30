/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>

#include "CartesianFrame.h"
#include "FullColumn.h"     //FColDsptr is defined
#include "FullMatrix.h"     //FMatDsptr is defined

namespace MbD {
	class MarkerFrame;
	class EndFrameqc;

	class EndFramec : public CartesianFrame
	{
		//markerFrame rOeO aAOe 
	public:
		EndFramec();
		EndFramec(const std::string& str);

		FMatDsptr aAeO();
		System* root() override;
		void initialize() override;
		void setMarkerFrame(MarkerFrame* markerFrm);
		MarkerFrame* getMarkerFrame();
		void initializeLocally() override;
		virtual void initEndFrameqct();
		virtual void initEndFrameqct2();
		void calcPostDynCorrectorIteration() override;
		FColDsptr aAjOe(size_t j);
		double riOeO(size_t i);
		virtual FColDsptr rmeO();
		virtual FColDsptr rpep();
		virtual FColFMatDsptr pAOppE();
		virtual FMatDsptr aBOp();
		std::shared_ptr<EndFrameqc> newCopyEndFrameqc();
		virtual bool isEndFrameqc();

		MarkerFrame* markerFrame; //Use raw pointer when pointing backwards.
		FColDsptr rOeO = std::make_shared<FullColumn<double>>(3);
		FMatDsptr aAOe = FullMatrix<double>::identitysptr(3);
	};
	//using EndFrmsptr = std::shared_ptr<EndFramec>;
}

