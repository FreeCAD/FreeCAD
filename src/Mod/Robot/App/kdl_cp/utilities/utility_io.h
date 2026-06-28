// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************** 
 *  \author 
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version 
 *		ORO_Geometry V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: utility_io.h,v 1.1.1.1.2.3 2003/06/26 15:23:59 psoetens Exp $
 *		$Name:  $ 
 *
 *  \file   utility_io.h
 *     Included by most lrl-files to provide some general
 *     functions and macro definitions related to file/stream I/O.
 */

#pragma once

//#include <kdl/kdl-config.h>


// Standard includes
#include <iostream>
#include <iomanip>
#include <fstream>


namespace KDL {


/**
 * checks validity of basic io of is
 */
void _check_istream(std::istream& is);


/** 
 * Eats characters of the stream until the character delim is encountered
 * @param is a stream
 * @param delim eat until this character is encountered
 */
void Eat(std::istream& is, int delim );

/** 
 * Eats characters of the stream as long as they satisfy the description in descript
 * @param is a stream
 * @param descript description string. A sequence of spaces, tabs, 
 *           new-lines and comments is regarded as 1 space in the description string.
 */
void Eat(std::istream& is,const char* descript);

/**
 * Eats a word of the stream delimited by the letters in delim or space(tabs...)
 * @param is a stream
 * @param delim a string containing the delimmiting characters
 * @param storage for returning the word
 * @param maxsize a word can be maximally maxsize-1 long.
 */
void EatWord(std::istream& is,const char* delim,char* storage,int maxsize);

/** 
 * Eats characters of the stream until the character delim is encountered
 * similar to Eat(is,delim) but spaces at the end are not read.
 * @param is a stream
 * @param delim eat until this character is encountered
 */
void EatEnd( std::istream& is, int delim );




}