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

#include "Item.h"
#include "EulerParametersDot.h"

namespace MbD {
	class System;
	class PartFrame;

	class Part : public Item
	{
		//ToDo: ipX ipE m aJ partFrame pX pXdot pE pEdot mX mE mEdot pTpE ppTpEpE ppTpEpEdot 
	public:
		Part();
		Part(const std::string& str);
		System* root() override;
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void setqX(FColDsptr x);
		FColDsptr getqX();
		void setaAap(FMatDsptr mat);
		void setqE(FColDsptr x);
		FColDsptr getqE();
		void setqXdot(FColDsptr x);
		FColDsptr getqXdot();
		void setomeOpO(FColDsptr x);
		FColDsptr getomeOpO();
		void setqXddot(FColDsptr x);
		FColDsptr getqXddot();
		void setqEddot(FColDsptr x);
		FColDsptr getqEddot();
		void qX(FColDsptr x);
		FColDsptr qX();
		void qE(std::shared_ptr<EulerParameters<double>> x);
		std::shared_ptr<EulerParameters<double>> qE();
		void qXdot(FColDsptr x);
		FColDsptr qXdot();
		void omeOpO(FColDsptr x);
		FColDsptr omeOpO();
		void qXddot(FColDsptr x);
		FColDsptr qXddot();
		void qEddot(FColDsptr x);
		FColDsptr qEddot();
		FMatDsptr aAOp();
		FColDsptr alpOpO();
		
		void setSystem(System* sys);
		void asFixed();
		void postInput() override;
		void calcPostDynCorrectorIteration() override;

		void prePosIC() override;
		void prePosKine() override;
		size_t iqX();
		size_t iqE();
		void iqX(size_t eqnNo);
		void iqE(size_t eqnNo);
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
		void setqsudot(FColDsptr col) override;
		void setqsudotlam(FColDsptr col) override;
		void postPosICIteration() override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<size_t>> redundantEqnNos) override;
		void reactivateRedundantConstraints() override;
		void constraintsReport() override;
		void postPosIC() override;
		void preDyn() override;
		void storeDynState() override;
		void fillPosKineError(FColDsptr col) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void preVelIC() override;
		void postVelIC() override;
		void fillVelICError(FColDsptr col) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void preAccIC() override;
		void calcp();
//		void calcpdot();
		void calcmEdot();
		void calcpTpE();
		void calcppTpEpE();
		void calcppTpEpEdot();
		void calcmE();
		void fillAccICIterError(FColDsptr col) override;
		void fillAccICIterJacob(SpMatDsptr mat) override;
		std::shared_ptr<EulerParametersDot<double>> qEdot();
		void setqsuddotlam(FColDsptr col) override;
		std::shared_ptr<StateData> stateData() override;
		double suggestSmallerOrAcceptDynStepSize(double hnew) override;
		void postDynStep() override;
		void postAccIC() override;

		System* system;	//Use raw pointer when pointing backwards.
		size_t ipX = SIZE_MAX; 
		size_t ipE = SIZE_MAX; 
		double m = 0.0; 
		DiagMatDsptr aJ;
		std::shared_ptr<PartFrame> partFrame;
		FColDsptr pX;
		FColDsptr pXdot;
		FColDsptr pE;
		FColDsptr pEdot;
		DiagMatDsptr mX;
		FMatDsptr mE;
		FMatDsptr mEdot;
		FColDsptr pTpE;
		FMatDsptr ppTpEpE;
		FMatDsptr ppTpEpEdot;
	};
}

