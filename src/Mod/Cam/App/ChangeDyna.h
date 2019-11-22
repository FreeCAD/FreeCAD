/***************************************************************************
 *   Copyright (c) 2007                                                    *
 *   Joachim Zettler <Joachim.Zettler@gmx.de>                              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef CHANGEDYNA_H
#define CHANGEDYNA_H

#include <iostream>
#include <string>
#include <vector>

class CamExport ChangeDyna
{
public:
    ChangeDyna();
    bool Read(const std::string &filename);
    // ~ChangeDyna();
private:
    bool ReformatStream(const std::stringstream& astream, std::string& astring);
    bool ReadCurve(std::ifstream &input,std::ofstream &output);
    bool ReadTimes(std::ifstream &input2);
    std::vector<std::pair<float,float> > m_ProperTime;

};
#endif

