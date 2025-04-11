/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ItemIJ.h"

namespace MbD {
	class EndFramec;
	using EndFrmsptr = std::shared_ptr<EndFramec>;

	class KinematicIeJe : public ItemIJ
	{
		//frmI frmJ 
	public:
		KinematicIeJe();
		KinematicIeJe(EndFrmsptr frmi, EndFrmsptr frmj);

		bool isKineIJ() override;
		virtual void calc_value();
		virtual void calc_pvaluepXI();
		virtual void calc_pvaluepEI();
		virtual void calc_ppvaluepXIpXI();
		virtual void calc_ppvaluepXIpEI();
		virtual void calc_ppvaluepEIpEI();
		virtual void calc_pvaluepXJ();
		virtual void calc_pvaluepEJ();
		virtual void calc_ppvaluepXIpXJ();
		virtual void calc_ppvaluepXIpEJ();
		virtual void calc_ppvaluepEIpXJ();
		virtual void calc_ppvaluepEIpEJ();
		virtual void calc_ppvaluepXJpXJ();
		virtual void calc_ppvaluepXJpEJ();
		virtual void calc_ppvaluepEJpEJ();
		virtual void calc_pvaluepXK();
		virtual void calc_pvaluepEK();
		virtual void calc_ppvaluepXIpEK();
		virtual void calc_ppvaluepEIpEK();
		virtual void calc_ppvaluepXJpEK();
		virtual void calc_ppvaluepEJpEK();
		virtual void calc_ppvaluepEKpEK();
		virtual void calc_pvaluept();
		virtual void calc_ppvaluepXIpt();
		virtual void calc_ppvaluepEIpt();
		virtual void calc_ppvaluepXJpt();
		virtual void calc_ppvaluepEJpt();
		virtual void calc_ppvaluepXKpt();
		virtual void calc_ppvaluepEKpt();
		virtual void calc_ppvalueptpt();

		virtual FRowDsptr pvaluepXI();
		virtual FRowDsptr pvaluepEI();
		virtual FMatDsptr ppvaluepXIpXI();
		virtual FMatDsptr ppvaluepXIpEI();
		virtual FMatDsptr ppvaluepEIpEI();
		virtual FRowDsptr pvaluepXJ();
		virtual FRowDsptr pvaluepEJ();
		virtual FMatDsptr ppvaluepXIpXJ();
		virtual FMatDsptr ppvaluepXIpEJ();
		virtual FMatDsptr ppvaluepEIpXJ();
		virtual FMatDsptr ppvaluepEIpEJ();
		virtual FMatDsptr ppvaluepXJpXJ();
		virtual FMatDsptr ppvaluepXJpEJ();
		virtual FMatDsptr ppvaluepEJpEJ();
		virtual FRowDsptr pvaluepXK();
		virtual FRowDsptr pvaluepEK();
		virtual FMatDsptr ppvaluepXIpEK();
		virtual FMatDsptr ppvaluepEIpEK();
		virtual FMatDsptr ppvaluepXJpEK();
		virtual FMatDsptr ppvaluepEJpEK();
		virtual FMatDsptr ppvaluepEKpEK();
		virtual double pvaluept();
		virtual double ppvalueptpt();
		virtual FRowDsptr ppvaluepXIpt();
		virtual FRowDsptr ppvaluepEIpt();
		virtual FRowDsptr ppvaluepXJpt();
		virtual FRowDsptr ppvaluepEJpt();
		virtual FRowDsptr ppvaluepXKpt();
		virtual FRowDsptr ppvaluepEKpt();
		virtual double value();

	};
}

