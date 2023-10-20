/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTItem.h"

namespace MbD {
    class EXPORT ASMTItemIJ : public ASMTItem
    {
        //
    public:
        ASMTItemIJ();
        void initialize() override;
        void setMarkerI(std::string mkrI);
        void setMarkerJ(std::string mkrJ);
        void readMarkerI(std::vector<std::string>& lines);
        void readMarkerJ(std::vector<std::string>& lines);
        void readFXonIs(std::vector<std::string>& lines);
        void readFYonIs(std::vector<std::string>& lines);
        void readFZonIs(std::vector<std::string>& lines);
        void readTXonIs(std::vector<std::string>& lines);
        void readTYonIs(std::vector<std::string>& lines);
        void readTZonIs(std::vector<std::string>& lines);
        void storeOnLevel(std::ofstream& os, int level) override;
        void storeOnTimeSeries(std::ofstream& os) override;

        std::string markerI, markerJ;
        FRowDsptr fxs, fys, fzs, txs, tys, tzs;
        FRowDsptr infxs, infys, infzs, intxs, intys, intzs;

    };
}

