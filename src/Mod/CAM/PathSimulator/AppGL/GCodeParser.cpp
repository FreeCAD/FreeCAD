/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "GCodeParser.h"
#include <ctype.h>
#include <stdio.h>

using namespace MillSim;

static char TokTypes[] = "GTXYZIJK";

GCodeParser::~GCodeParser()
{
	// Clear the vector
	Operations.clear();
}

bool GCodeParser::Parse(const char* filename)
{
	Operations.clear();
	lastState = { eNop, -1, 0, 0, 0, 0, 0, 0 };
	lastTool = -1;

	FILE* fl;
	if ((fl = fopen(filename, "rt")) == nullptr)
		return false;

	char line[120];

	while (!feof(fl))
	{
		if (fgets(line, 120, fl) != NULL)
		{
			AddLine(line);
		}
	}
	fclose(fl);
	return false;
}

const char* GCodeParser::GetNextToken(const char* ptr, GCToken* token)
{
	float tokval;
	token->letter = '*';
	while (*ptr != 0)
	{
		char letter = toupper(*ptr);
		ptr++;

		if (letter == ' ')
			continue;
		if (letter == '(')
			break;
			
		if (IsValidTok(letter))
		{
			ptr = ParseFloat(ptr, &tokval);
			token->letter = letter;
			token->fval = tokval;
			token->ival = (int)(tokval + 0.5);
			break;
		}
	}
	return ptr;
}

bool GCodeParser::IsValidTok(char tok)
{
	int len = (int)strlen(TokTypes);
	for (int i = 0; i < len; i++)
		if (tok == TokTypes[i])
			return true;
	return false;
}

const char* GCodeParser::ParseFloat(const char* ptr, float* retFloat)
{
	float decPos = 10;
	float sign = 1;
	bool decimalPointFound = false;
	float res = 0;
	while (*ptr != 0)
	{
		char letter = toupper(*ptr);
		ptr++;

		if (letter == ' ')
			continue;

		if (letter == '-')
			sign = -1;
		else if (letter == '.')
			decimalPointFound = true;
		else if (letter >= '0' && letter <= '9')
		{
			float digitVal = (float)(letter - '0');
			if (decimalPointFound)
			{
				res = res + digitVal / decPos;
				decPos *= 10;
			}
			else
				res = res * 10 + digitVal;
		}
		else
		{
			ptr--;
			break;
		}
	}
	*retFloat = res * sign;
	return ptr;
}

bool GCodeParser::ParseLine(const char* ptr)
{
	GCToken token;
	bool validMotion = false;
	bool exitLoop = false;
	while (*ptr != 0 && !exitLoop)
	{
		ptr = GetNextToken(ptr, &token);
		switch (token.letter)
		{
		case '*':
			exitLoop = true;
			break;

		case 'G':
			if (token.ival == 0 || token.ival == 1)
				lastState.cmd = eMoveLiner;
			else if (token.ival == 2)
				lastState.cmd = eRotateCW;
			else if (token.ival == 3)
				lastState.cmd = eRotateCCW;
			break;

		case 'T':
			lastState.tool = token.ival;
			break;

		case 'X':
			lastState.x = token.fval;
			validMotion = true;
			break;

		case 'Y':
			lastState.y = token.fval;
			validMotion = true;
			break;

		case 'Z':
			lastState.z = token.fval;
			validMotion = true;
			break;

		case 'I':
			lastState.i = token.fval;
			break;

		case 'J':
			lastState.j = token.fval;
			break;

		case 'K':
			lastState.k = token.fval;
			break;
		}
	}
	return validMotion;
}

bool GCodeParser::AddLine(const char* ptr)
{
	bool res = ParseLine(ptr);
	if (res)
		Operations.push_back(lastState);
	return res;
}
