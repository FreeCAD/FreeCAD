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
 *		$Id: utility_io.cpp,v 1.1.1.1.2.4 2003/06/26 15:23:59 psoetens Exp $
 *		$Name:  $ 
 * \todo
 *   make IO routines more robust against the differences between DOS/UNIX end-of-line style.
 ****************************************************************************/


#include "utility_io.h"
#include "error.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

namespace KDL {

//
//  _functions are private functions 
//

    void _check_istream(std::istream& is)
    {
        if ((!is.good())&&(is.eof()) )
            {
            throw Error_BasicIO_File();
            }
    }
// Eats until the end of the line
	int _EatUntilEndOfLine( std::istream& is, int* countp=NULL) {
    int ch;
    int count;
    count = 0;
    do {
        ch = is.get();
        count++;
        _check_istream(is);
    } while (ch!='\n');
    if (countp!=NULL) *countp = count;
    return ch;
}

// Eats until the end of the comment
	int _EatUntilEndOfComment( std::istream& is, int* countp=NULL) {
    int ch;
    int count;
    count = 0;
    int prevch;
    ch = 0;
    do {
        prevch = ch;
        ch = is.get();
        count++;
        _check_istream(is);
        if ((prevch=='*')&&(ch=='/')) {
            break;
        }
    } while (true);
    if (countp!=NULL) *countp = count;
    ch = is.get();
    return ch;
}

// Eats space-like characters and comments
// possibly returns the number of space-like characters eaten.
int _EatSpace( std::istream& is,int* countp=NULL) {
    int ch;
    int count;
    count=-1;
    do { 
        _check_istream(is);

        ch = is.get(); 
        count++;
        if (ch == '#') {
            ch = _EatUntilEndOfLine(is,&count);
        }
        if (ch == '/') {
            ch = is.get();
            if (ch == '/') {
                ch = _EatUntilEndOfLine(is,&count);
            } else  if (ch == '*') {
                ch = _EatUntilEndOfComment(is,&count);
            } else {
                is.putback(ch);
                ch = '/';
            }
        }
    } while ((ch==' ')||(ch=='\n')||(ch=='\t'));
    if (countp!=NULL) *countp =  count;
    return ch;
}



// Eats whites, returns, tabs and the delim character
//  Checks wether delim char. is encountered.
void Eat( std::istream& is, int delim )
{   
    int ch;
    ch=_EatSpace(is);
    if (ch != delim) {
       throw Error_BasicIO_Exp_Delim();
    }
    ch=_EatSpace(is);   
    is.putback(ch);
}

// Eats whites, returns, tabs and the delim character
//  Checks wether delim char. is encountered.
// EatEnd does not eat all space-like char's at the end.
void EatEnd( std::istream& is, int delim )
{   
    int ch;
    ch=_EatSpace(is);
    if (ch != delim) {
       throw Error_BasicIO_Exp_Delim();
    }
}



// For each space in descript, this routine eats whites,tabs, and newlines (at least one)
// There should be no consecutive spaces in the description.
// for each letter in descript, its reads the corresponding letter in the output
// the routine is case insensitive.


// Simple routine, enough for our purposes.
//  works with ASCII chars
inline char Upper(char ch) 
{
    /*if (('a'<=ch)&&(ch<='z'))
        return (ch-'a'+'A');
    else
        return ch;
    */
    return toupper(ch);
}

void Eat(std::istream& is,const char* descript)
{
    // eats whites before word
    char ch;
    char chdescr;
    ch=_EatSpace(is);   
    is.putback(ch);
    const char* p;
    p = descript;
    while ((*p)!=0) {
        chdescr = (char)Upper(*p);
        if (chdescr==' ') {
            int count=0;
            ch=_EatSpace(is,&count);
            is.putback(ch);
            if (count==0) {
                throw Error_BasicIO_Not_A_Space();
            }
        } else {
            ch=(char)is.get();
            if (chdescr!=Upper(ch)) {
               throw Error_BasicIO_Unexpected();
            }
        }
        p++;
    }
    
}



void EatWord(std::istream& is,const char* delim,char* storage,int maxsize)
{
    int ch;
    char* p;
    int size;
    // eat white before word
    ch=_EatSpace(is);
    p = storage;
    size=0;
    int count = 0;
    while ((count==0)&&(strchr(delim,ch)==NULL)) {
        *p = (char) toupper(ch);
        ++p;
        if (size==maxsize) {
           throw Error_BasicIO_ToBig();
        }
        _check_istream(is);
        ++size;
        //ch = is.get();
        ch =_EatSpace(is,&count);
    }
    *p=0;
    is.putback(ch);
}


}
