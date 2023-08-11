/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <string>

#include "Symbolic.h"
namespace MbD {
    class Variable : public Symbolic
    {
        //name value 
	public:
		Variable();
		Variable(const char* str);
		Variable(double val);
		void initialize() override;
		void setName(std::string& str);
		const std::string& getName() const;
		double getValue() override;
		std::ostream& printOn(std::ostream& s) const override;
		virtual void setValue(double val);

		std::string name;
		double value;
	};
}

