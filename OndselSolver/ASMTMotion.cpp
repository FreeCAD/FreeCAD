/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTMotion.h"

namespace MbD {
    void ASMTMotion::readMotionSeries(std::vector<std::string>& lines)
    {
        std::string str = lines[0];
        std::string substr = "MotionSeries";
        auto pos = str.find(substr);
        assert(pos != std::string::npos);
        str.erase(0, pos + substr.length());
        auto seriesName = readString(str);
        assert(fullName("") == seriesName);
        lines.erase(lines.begin());
        readFXonIs(lines);
        readFYonIs(lines);
        readFZonIs(lines);
        readTXonIs(lines);
        readTYonIs(lines);
        readTZonIs(lines);
    }

    void ASMTMotion::initMarkers()
    {
    }

    void ASMTMotion::storeOnLevel(std::ofstream& os, int level)
    {
        assert(false);
    }

    void ASMTMotion::storeOnTimeSeries(std::ofstream& os)
    {
        assert(false);
    }
}
