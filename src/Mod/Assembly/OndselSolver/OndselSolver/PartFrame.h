/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

#include "CartesianFrame.h"
#include "EndFramec.h"
#include "FullColumn.h"
#include "EulerParameters.h"
#include "EulerParametersDot.h"
#include "CREATE.h"

namespace MbD {
	class Part;
	class MarkerFrame;
	class EulerConstraint;
	class AbsConstraint;

	class PartFrame : public CartesianFrame
	{
		//ToDo: part iqX iqE qX qE qXdot qEdot qXddot qEddot aGeu aGabs markerFrames 
	public:
		PartFrame();
		PartFrame(const std::string& str);
		System* root() override;
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void asFixed();
		void postInput() override;
		void calcPostDynCorrectorIteration() override;

		void setqX(FColDsptr x);
		FColDsptr getqX();
		void setqE(FColDsptr x);
		void setaAap(FMatDsptr mat);
		FColDsptr getqE();
		void setqXdot(FColDsptr x);
		FColDsptr getqXdot();
		void setomeOpO(FColDsptr x);
		FColDsptr getomeOpO();
		void setqXddot(FColDsptr x);
		FColDsptr getqXddot();
		void setqEddot(FColDsptr x);
		FColDsptr getqEddot();
		FColDsptr omeOpO();

		void setPart(Part* x);
		Part* getPart();
		void addMarkerFrame(std::shared_ptr<MarkerFrame> x);
		EndFrmsptr endFrame(const std::string& name);
		void aGabsDo(const std::function <void(std::shared_ptr<Constraint>)>& f);
		void markerFramesDo(const std::function <void(std::shared_ptr<MarkerFrame>)>& f);
		void removeRedundantConstraints(std::shared_ptr<std::vector<size_t>> redundantEqnNos) override;
		void reactivateRedundantConstraints() override;
		void constraintsReport() override;

		void prePosIC() override;
		void prePosKine() override;
		FColDsptr rOpO();
		FMatDsptr aAOp();
		FMatDsptr aC();
		FMatDsptr aCdot();
		FColDsptr alpOpO();
		FColFMatDsptr pAOppE();
		void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) override;
		void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) override;
		void fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints) override;
		void fillqsu(FColDsptr col) override;
		void fillqsuWeights(DiagMatDsptr diagMat) override;
		void fillqsuddotlam(FColDsptr col) override;
		void fillqsulam(FColDsptr col) override;
		void fillqsudot(FColDsptr col) override;
		void fillqsudotWeights(DiagMatDsptr diagMat) override;
		void useEquationNumbers() override;
		void setqsu(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		void setqsudotlam(FColDsptr col) override;
		void setqsudot(FColDsptr col) override;
		void setqsuddotlam(FColDsptr col) override;
		void postPosICIteration() override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void postPosIC() override;
		void preDyn() override;
		void storeDynState() override;
		void fillPosKineError(FColDsptr col) override;
		void preVelIC() override;
		void postVelIC() override;
		void fillVelICError(FColDsptr col) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void preAccIC() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillAccICIterJacob(SpMatDsptr mat) override;
		FMatDsptr aBOp();
		void fillPosKineJacob(SpMatDsptr mat) override;
		double suggestSmallerOrAcceptDynStepSize(double hnew) override;
		void postDynStep() override;

		Part* part = nullptr; //Use raw pointer when pointing backwards.
		size_t iqX = SIZE_MAX;
		size_t iqE = SIZE_MAX;	//Position index of frame variables qX and qE in system list of variables
		FColDsptr qX = std::make_shared<FullColumn<double>>(3);
		std::shared_ptr<EulerParameters<double>> qE = CREATE<EulerParameters<double>>::With(4);
		FColDsptr qXdot = std::make_shared<FullColumn<double>>(3);
		std::shared_ptr<EulerParametersDot<double>> qEdot = CREATE<EulerParametersDot<double>>::With(4);
		FColDsptr qXddot = std::make_shared<FullColumn<double>>(3);
		FColDsptr qEddot = std::make_shared<FullColumn<double>>(4);
		std::shared_ptr<Constraint> aGeu;
		std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> aGabs;
		std::shared_ptr<std::vector<std::shared_ptr<MarkerFrame>>> markerFrames;
	};
}

