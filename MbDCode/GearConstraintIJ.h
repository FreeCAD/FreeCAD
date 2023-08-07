#pragma once

#include "ConstraintIJ.h"
#include "OrbitAngleZIecJec.h"

namespace MbD {
	class GearConstraintIJ : public ConstraintIJ
	{
		//orbitIeJe orbitJeIe radiusI radiusJ 
	public:
		GearConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj);

		void calcPostDynCorrectorIteration() override;
		void initialize() override;
		void initializeGlobally() override;
		void initializeLocally() override;
		virtual void initorbitsIJ();
		void postInput() override;
		void postPosICIteration() override;
		void preAccIC() override;
		void prePosIC() override;
		void preVelIC() override;
		double ratio();
		void simUpdateAll() override;

		std::shared_ptr<OrbitAngleZIecJec> orbitIeJe, orbitJeIe;
		double radiusI, radiusJ;
	};
}

