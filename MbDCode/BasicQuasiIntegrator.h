#pragma once

#include "BasicIntegrator.h"

namespace MbD {
	class BasicQuasiIntegrator : public BasicIntegrator
	{
		//
	public:
        void firstStep();
        //bool isRedoingFirstStep();
        //bool isRedoingStep();
        void nextStep();
        //void reportStepStats();
        //void reportTrialStepStats();
        void runInitialConditionTypeSolution() override;
        void selectFirstStepSize();
        //void selectStepSize();
	};
}

