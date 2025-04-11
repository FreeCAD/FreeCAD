/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "ConstraintIJ.h"
#include "DispCompIecJecKec.h"

namespace MbD {
	class TranslationConstraintIJ : public ConstraintIJ
	{
		//riIeJeIe
	public:
		TranslationConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi);

		static std::shared_ptr<TranslationConstraintIJ> With(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi);

		void calcPostDynCorrectorIteration() override;
		virtual void initriIeJeIe();
		void initialize() override;
		void initializeGlobally() override;
		void initializeLocally() override;
		void postInput() override;
		void postPosICIteration() override;
		void preAccIC() override;
		void prePosIC() override;
		void preVelIC() override;
		void simUpdateAll() override;
		ConstraintType type() override;

		size_t axisI;
		std::shared_ptr<DispCompIecJecKec> riIeJeIe;
		//ToDo: Use DispCompIecJecIe instead of DispCompIecJecKec
	};
}

