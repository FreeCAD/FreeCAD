#pragma once

#include "ConstraintIJ.h"
#include "DispCompIecJecIe.h"
#include "AngleZIecJec.h"

namespace MbD {
	class RackPinConstraintIJ : public ConstraintIJ
	{
		//xIeJeIe thezIeJe pitchRadius 
	public:
        RackPinConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void init_xthez();
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        void simUpdateAll() override;

		std::shared_ptr<DispCompIecJecIe> xIeJeIe;
		std::shared_ptr<AngleZIecJec> thezIeJe;
		double pitchRadius;
	};
}

