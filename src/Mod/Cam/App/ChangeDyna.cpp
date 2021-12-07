/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
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


#include "PreCompiled.h"
#include "ChangeDyna.h"
#include <sstream>
#include <vector>
#include <fstream>
#include <stdexcept>

ChangeDyna::ChangeDyna()
{
    m_ProperTime.clear();
}
bool ChangeDyna::Read( const std::string & _filename)
{
    // open file for reading
    std::ifstream input( _filename.c_str() );
    std::ifstream input2("CurveTimes.k");
    if (!input2.is_open()) return false;
    if (!ReadTimes(input2)) return false;
    std::ofstream output("dyna2.str");
    if ( !input.is_open() )
    {
        std::cout << "failed to open file" << std::endl;
        return false;
    }
    if ( !ReadCurve( input, output ) )
    {
        std::cout << "failed to read curve_data" << std::endl;
        return false;
    }

    input.close();
    output.close();
    input2.close();
    return true;
}


bool ChangeDyna::ReadTimes(std::ifstream &input2)
{
    input2.seekg(std::ifstream::beg);
    std::string line;
    unsigned int i=0;
    std::pair<float,float> tempPair;
    std::stringstream astream1;
    do
    {
        std::getline(input2,line);
        if (line.size() == 0) continue;
        astream1.str(line);
        astream1 >> tempPair.first >> tempPair.second;
        m_ProperTime.push_back(tempPair);
        astream1.str("");
        astream1.clear();

    }
    while (input2.good());
    return true;
}


bool ChangeDyna::ReadCurve(std::ifstream &input,std::ofstream &output)
{
    input.seekg( std::ifstream::beg );
    std::string line,subline1;
    bool found = false;
    int current_index;
    do
    {
        std::getline( input, line );
        if (line.size() == 0) //if we have an empty line
        {
            continue;
        }
        if (found)
        {
            //If we find a new Keyword description
            if (line.size() > 3 && line.at(0) == '$' && (line.find("nid") == std::string::npos))
            {
                found = false;
                output << line << std::endl;
                continue; //go directly to the next line
            }
            else if (line.at(0) == '$')
            {
                output << line << std::endl;
                continue;
            }
            else // Now we change the Birth and Death-Times
            {
                std::stringstream astream1,astream2;
                astream1.precision(20);
                astream2.precision(20);
                if (line.at(9) != '0') //If we are at the first line
                {
                    //Extract the Curve-Index
                    astream1.str(line.substr(10,5));
                    astream1 >> current_index;
                    //Exchange the Death time. We need a vector of pairs (birth,death)
                    if ((current_index-2) < 0)
                        return false;
                    astream2 << m_ProperTime[current_index-2].second;
                    //Now we have to reformat the string to fit exactly 9 digits
                    try
                    {
                        ReformatStream(astream2,subline1);
                        output << line.substr(0,66) << subline1 << line.substr(75,5) << std::endl;
                    }
                    catch (const std::out_of_range&)
                    {
                        output << line << std::endl;
                        return false;
                    }

                    continue;
                }
                else //we are at the second line and can exchange the Birth-Time
                {

                    astream2 << m_ProperTime[current_index-2].first;
                    try
                    {
                        ReformatStream(astream2,subline1);
                        output << line.substr(0,31) << subline1 << std::endl;
                    }
                    catch (std::out_of_range)
                    {
                        output << line << std::endl;
                        return false;
                    }

                    continue;
                }
            }
        }
        else
        {
            std::string search("Velocity/Acceleration/");
            if (line.find(search)!=std::string::npos)
                found = true;
            output << line << std::endl;
            continue;
        }
    }
    while ( input.good());
    return true;
}


bool ChangeDyna::ReformatStream(const std::stringstream& astream, std::string& astring)
{
    astring.clear();
    std::string tempstring(astream.str());
    unsigned int found=tempstring.find('.');
    astring = tempstring.substr(0,found);
    astring += '.';
    //Now add the rest. We have only space for 9 digits in total (the '-' is already there)
    astring += tempstring.substr(found+1,9-astring.size());
    while (astring.size() < 9)
        astring += '0'; //We add '0' until we have a size of 9
    return true;
}
