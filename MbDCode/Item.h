#pragma once

#include <string>
#include <vector>

#include "FullColumn.h"
#include "FullMatrix.h"
#include "DiagonalMatrix.h"
#include "SparseMatrix.h"
#include "enum.h"

namespace MbD {
	class System;
	class Constraint;
	class StateData;

	class Item
	{
		//name
	public:
		Item();
		Item(const char* str);
		virtual System* root();

		virtual void calcPostDynCorrectorIteration();
		virtual void checkForCollisionDiscontinuityBetweenand(double impulsePrevious, double impulse);
		virtual double checkForDynDiscontinuityBetweenand(double tprev, double t);
		virtual std::string classname();
		virtual void constraintsReport();
		virtual void discontinuityAtaddTypeTo(double t, std::shared_ptr<std::vector<DiscontinuityType>> disconTypes);
		virtual void discontinuityAtICAddTo(std::shared_ptr<std::vector<DiscontinuityType>> disconTypes);
		virtual void fillAccICIterError(FColDsptr col);
		virtual void fillAccICIterJacob(SpMatDsptr mat);
		virtual void fillCollisionDerivativeICError(FColDsptr col);
		virtual void fillCollisionDerivativeICJacob(SpMatDsptr mat);
		virtual void fillCollisionError(FColDsptr col);
		virtual void fillCollisionpFpy(SpMatDsptr mat);
		virtual void fillCollisionpFpydot(SpMatDsptr mat);
		virtual void fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints);
		virtual void fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
		virtual void fillDynError(FColDsptr col);
		virtual void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints);
		virtual void fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);
		virtual void fillpFpy(SpMatDsptr mat);
		virtual void fillpFpydot(SpMatDsptr mat);
		virtual void fillPosICError(FColDsptr col);
		virtual void fillPosICJacob(FMatDsptr mat);
		virtual void fillPosICJacob(SpMatDsptr mat);
		virtual void fillPosKineError(FColDsptr col);
		virtual void fillPosKineJacob(SpMatDsptr mat);
		virtual void fillpqsumu(FColDsptr col);
		virtual void fillpqsumudot(FColDsptr col);
		virtual void fillqsu(FColDsptr col);
		virtual void fillqsuddotlam(FColDsptr col);
		virtual void fillqsudot(FColDsptr col);
		virtual void fillqsudotPlam(FColDsptr col);
		virtual void fillqsudotPlamDeriv(FColDsptr col);
		virtual void fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat);
		virtual void fillqsulam(FColDsptr col);
		virtual void fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat);
		virtual void fillqsuWeightsSmall(FColDsptr col);
		virtual void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints);
		virtual void fillStaticError(FColDsptr col);
		virtual void fillStaticJacob(FMatDsptr mat);
		virtual void fillVelICError(FColDsptr col);
		virtual void fillVelICJacob(SpMatDsptr mat);
		virtual void getString(std::string str);
		virtual void initialize();
		virtual void initializeGlobally();
		virtual void initializeLocally();
		virtual bool isJointForce();
		virtual bool isJointTorque();
		virtual bool isKinedotIJ();
		virtual bool isKineIJ();
		virtual void logString(std::string& str);
		void logString(const char* chars);
		virtual void logStringwithArgument(const char* chars, const char* chars1);
		virtual void logStringwithArguments(const char* chars, std::shared_ptr<std::vector<char*>> arrayOfChars);
		virtual void normalImpulse(double imp);
		virtual void postAccIC();
		virtual void postAccICIteration();
		virtual void postCollisionCorrector();
		virtual void postCollisionCorrectorIteration();
		virtual void postCollisionDerivativeIC();
		virtual void postCollisionPredictor();
		virtual void postCollisionStep();
		virtual void postDyn();
		virtual void postDynCorrector();
		virtual void postDynCorrectorIteration();
		virtual void postDynFirstStep();
		virtual void postDynOutput();
		virtual void postDynPredictor();
		virtual void postDynStep();
		virtual void postInput();
		virtual void postPosIC();
		virtual void postPosICIteration();
		virtual void postStatic();
		virtual void postStaticIteration();
		virtual void postVelIC();
		virtual void preAccIC();
		virtual void preCollision();
		virtual void preCollisionCorrector();
		virtual void preCollisionCorrectorIteration();
		virtual void preCollisionDerivativeIC();
		virtual void preCollisionPredictor();
		virtual void preCollisionStep();
		virtual void preDyn();
		virtual void preDynCorrector();
		virtual void preDynCorrectorIteration();
		virtual void preDynFirstStep();
		virtual void preDynOutput();
		virtual void preDynPredictor();
		virtual void preDynStep();
		virtual void preICRestart();
		virtual void prePosIC();
		virtual void prePosKine();
		virtual void preStatic();
		virtual void preVelIC();
		virtual void reactivateRedundantConstraints();
		virtual void registerName();
		virtual void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos);
		virtual void setpqsumu(FColDsptr col);
		virtual void setpqsumuddot(FColDsptr col);
		virtual void setpqsumudot(FColDsptr col);
		virtual void setqsu(FColDsptr qsuOld);
		virtual void setqsuddotlam(FColDsptr col);
		virtual void setqsudot(FColDsptr col);
		virtual void setqsudotlam(FColDsptr col);
		virtual void setqsudotPlam(FColDsptr col);
		virtual void setqsudotPlamDeriv(FColDsptr col);
		virtual void setqsulam(FColDsptr col);
		virtual void simUpdateAll();
		virtual std::shared_ptr<StateData> stateData();
		virtual void storeCollisionState();
		virtual void storeDynState();
		virtual void submitToSystem(std::shared_ptr<Item> self);
		virtual double suggestSmallerOrAcceptCollisionFirstStepSize(double hnew);
		virtual double suggestSmallerOrAcceptCollisionStepSize(double hnew);
		virtual double suggestSmallerOrAcceptDynFirstStepSize(double hnew);
		virtual double suggestSmallerOrAcceptDynStepSize(double hnew);
		virtual void useEquationNumbers();

		virtual std::ostream& printOn(std::ostream& s) const;
		friend std::ostream& operator<<(std::ostream& s, const Item& item)
		{
			if (&item) {
				return item.printOn(s);
			}
			else {
				s << "NULL";
			}
			return s;
		}

		std::string name;
		Item* owner = nullptr;	//Use raw pointer when pointing backwards.
	};
}

