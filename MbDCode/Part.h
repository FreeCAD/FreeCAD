#pragma once

#include <memory>

#include "Item.h"
#include "FullColumn.h"
#include "FullMatrix.h"
#include "EulerParametersDot.h"

namespace MbD {
	class System;
	class PartFrame;
	template<typename T> class DiagonalMatrix;

	class Part : public Item
	{
		//ToDo: ipX ipE m aJ partFrame pX pXdot pE pEdot mX mE mEdot pTpE ppTpEpE ppTpEpEdot 
	public:
		Part();
		Part(const char* str);
		System* root() override;
		void initialize() override;
		void initializeLocally() override;
		void initializeGlobally() override;
		void setqX(FColDsptr x);
		FColDsptr getqX();
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
		void iqX(int eqnNo);
		void iqE(int eqnNo);
		void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) override;
		void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) override;
		void fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints) override;
		void fillqsu(FColDsptr col) override;
		void fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat) override;
		void fillqsuddotlam(FColDsptr col) override;
		void fillqsulam(FColDsptr col) override;
		void fillqsudot(FColDsptr col) override;
		void fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat) override;
		void useEquationNumbers() override;
		void setqsu(FColDsptr col) override;
		void setqsulam(FColDsptr col) override;
		void setqsudot(FColDsptr col) override;
		void setqsudotlam(FColDsptr col) override;
		void postPosICIteration() override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos) override;
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
		void calcpdot();
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
		void submitToSystem(std::shared_ptr<Item> self) override;

		System* system;	//Use raw pointer when pointing backwards.
		int ipX = -1; 
		int ipE = -1; 
		double m = 0.0; 
		std::shared_ptr<DiagonalMatrix<double>> aJ;
		std::shared_ptr<PartFrame> partFrame;
		FColDsptr pX;
		FColDsptr pXdot;
		FColDsptr pE;
		FColDsptr pEdot;
		std::shared_ptr<DiagonalMatrix<double>> mX;
		FMatDsptr mE;
		FMatDsptr mEdot;
		FColDsptr pTpE;
		FMatDsptr ppTpEpE;
		FMatDsptr ppTpEpEdot;
	};
}

