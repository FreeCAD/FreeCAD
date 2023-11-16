/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ConstantGravity.h"
#include "System.h"
#include "Part.h"

using namespace MbD;

void MbD::ConstantGravity::fillAccICIterError(FColDsptr col)
{
	for (auto& part : *(root()->parts)) {
		col->atiplusFullColumntimes(part->iqX(), gXYZ, part->m);
	}
}
