/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "DirectionCosineConstraintIqctJqc.h"

namespace MbD {

    class AllowZRotationConstraintIqctJqc : public DirectionCosineConstraintIqctJqc
    {
    public:
        AllowZRotationConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj);
        static std::shared_ptr<AllowZRotationConstraintIqctJqc> With(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj);

        void postInput() override;
        void postPosIC() override;

    };
}