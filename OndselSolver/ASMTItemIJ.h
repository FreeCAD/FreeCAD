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
    class ASMTItemIJ : public ASMTItem
    {
        //
    public:
        ASMTItemIJ();
        void initialize() override;
        void setMarkerI(const std::string& mkrI);
        void setMarkerJ(const std::string& mkrJ);
        void readMarkerI(std::vector<std::string>& lines);
        void readMarkerJ(std::vector<std::string>& lines);
        void readFXonIs(std::vector<std::string>& lines);
        void readFYonIs(std::vector<std::string>& lines);
        void readFZonIs(std::vector<std::string>& lines);
        void readTXonIs(std::vector<std::string>& lines);
        void readTYonIs(std::vector<std::string>& lines);
        void readTZonIs(std::vector<std::string>& lines);
        void storeOnLevel(std::ofstream& os, size_t level) override;
        void storeOnTimeSeries(std::ofstream& os) override;
        void parseASMT(std::vector<std::string>& lines) override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        virtual std::shared_ptr<ItemIJ> mbdClassNew();

        std::string markerI, markerJ;
        FRowDsptr fxs, fys, fzs, txs, tys, tzs;
        FRowDsptr infxs, infys, infzs, intxs, intys, intzs;

    };
}

