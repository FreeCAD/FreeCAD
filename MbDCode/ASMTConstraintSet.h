#pragma once

#include "ASMTItem.h"

namespace MbD {
    class ASMTConstraintSet : public ASMTItem
    {
        //
    public:
        void readMarkerI(std::vector<std::string>& lines);
        void readMarkerJ(std::vector<std::string>& lines);
        void readFXonIs(std::vector<std::string>& lines);
        void readFYonIs(std::vector<std::string>& lines);
        void readFZonIs(std::vector<std::string>& lines);
        void readTXonIs(std::vector<std::string>& lines);
        void readTYonIs(std::vector<std::string>& lines);
        void readTZonIs(std::vector<std::string>& lines);

        std::string markerI, markerJ;
        FRowDsptr fxs, fys, fzs, txs, tys, tzs;

    };
}

