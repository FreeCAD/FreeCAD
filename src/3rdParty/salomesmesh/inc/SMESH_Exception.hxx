//  SALOME Utils : general SALOME's definitions and tools
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
// 
//  This library is free software; you can redistribute it and/or 
//  modify it under the terms of the GNU Lesser General Public 
//  License as published by the Free Software Foundation; either 
//  version 2.1 of the License. 
// 
//  This library is distributed in the hope that it will be useful, 
//  but WITHOUT ANY WARRANTY; without even the implied warranty of 
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
//  Lesser General Public License for more details. 
// 
//  You should have received a copy of the GNU Lesser General Public 
//  License along with this library; if not, write to the Free Software 
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
// 
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com

#if !defined( __SMESH_Exception_hxx__ )
#define __SMESH_Exception_hxx__

# include <exception>
# include <iostream>

#ifdef _DEBUG_
# define LOCALIZED(message) #message , __FILE__ , __LINE__
#else
# define LOCALIZED(message) #message
#endif

#if defined SMESH_EXPORTS
#if defined WIN32
#define SMESH_EXPORT __declspec( dllexport )
#else
#define SMESH_EXPORT
#endif
#else
#if defined WNT
#define SMESH_EXPORT __declspec( dllimport )
#else
#define SMESH_EXPORT
#endif
#endif

class SMESH_EXPORT SMESH_Exception : public std::exception
{

private :
	SMESH_Exception( void );

protected :
	const char* _text ;	// non constant pointer but read only char variable

public :
	SMESH_Exception( const char *text, const char *fileName=0, const unsigned int lineNumber=0 );
	SMESH_Exception( const SMESH_Exception &ex );
	~SMESH_Exception() throw ();
	friend std::ostream & operator<<( std::ostream &os , const SMESH_Exception &ex );
	virtual const char *what( void ) const throw () ;
} ;


#endif		/* #if !defined( __SMESH_Exception_hxx__ ) */
