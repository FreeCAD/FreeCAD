/*------------------------------------------------------
   CCmdLine

   A utility for parsing command lines.

   Copyright (C) 1999 Chris Losinger, Smaller Animals Software.
   http://www.smalleranimals.com

   This software is provided 'as-is', without any express
   or implied warranty.  In no event will the authors be 
   held liable for any damages arising from the use of this software.

   Permission is granted to anyone to use this software 
   for any purpose, including commercial applications, and 
   to alter it and redistribute it freely, subject to the 
   following restrictions:

     1. The origin of this software must not be misrepresented; 
   you must not claim that you wrote the original software. 
   If you use this software in a product, an acknowledgment 
   in the product documentation would be appreciated but is not required.
   
     2. Altered source versions must be plainly marked as such, 
   and must not be misrepresented as being the original software.
   
     3. This notice may not be removed or altered from any source 
   distribution.

  -------------------------

   Example :

   Our example application uses a command line that has two
   required switches and two optional switches. The app should abort
   if the required switches are not present and continue with default
   values if the optional switches are not present.

      Sample command line : 
      MyApp.exe -p1 text1 text2 -p2 "this is a big argument" -opt1 -55 -opt2

      Switches -p1 and -p2 are required. 
      p1 has two arguments and p2 has one.
      
      Switches -opt1 and -opt2 are optional. 
      opt1 requires a numeric argument. 
      opt2 has no arguments.
      
      Also, assume that the app displays a 'help' screen if the '-h' switch
      is present on the command line.

   #include "CmdLine.h"

   void main(int argc, char **argv)
   {
      // our cmd line parser object
      CCmdLine cmdLine;

      // parse argc,argv 
      if (cmdLine.SplitLine(argc, argv) < 1)
      {
         // no switches were given on the command line, abort
         ASSERT(0);
         exit(-1);
      }

      // test for the 'help' case
      if (cmdLine.HasSwitch("-h"))
      {
         show_help();
         exit(0);
      }

      // get the required arguments
      StringType p1_1, p1_2, p2_1;
      try
      {  
         // if any of these fail, we'll end up in the catch() block
         p1_1 = cmdLine.GetArgument("-p1", 0);
         p1_2 = cmdLine.GetArgument("-p1", 1);
         p2_1 = cmdLine.GetArgument("-p2", 0);

      }
      catch (...)
      {
         // one of the required arguments was missing, abort
         ASSERT(0);
         exit(-1);
      }

      // get the optional parameters

      // convert to an int, default to '100'
      int iOpt1Val =    atoi(cmdLine.GetSafeArgument("-opt1", 0, 100));

      // since opt2 has no arguments, just test for the presence of
      // the '-opt2' switch
      bool bOptVal2 =   cmdLine.HasSwitch("-opt2");

      .... and so on....

   }

   If this class is used in an MFC application, StringType is CString, else
   it uses the STL 'string' type.

   If this is an MFC app, you can use the __argc and __argv macros from
   you CYourWinApp::InitInstance() function in place of the standard argc 
   and argv variables. 

------------------------------------------------------*/
#ifndef SACMDSH
#define SACMDSH


#ifdef __AFX_H__
// if we're using MFC, use CStrings 
#define StringType CString
#else
// if we're not using MFC, use STL strings
#define StringType string
#endif

// tell the compiler to shut up
#pragma warning(disable:4786)

//#include <iostream> // you may need this
#include <map>
#include <string>
#include <vector>
using namespace std ;

// handy little container for our argument vector
struct CCmdParam
{
   vector<StringType> m_strings;
};

// this class is actually a map of strings to vectors
typedef map<StringType, CCmdParam> _CCmdLine;

// the command line parser class
class CCmdLine : public _CCmdLine
{

public:
   /*------------------------------------------------------
      int CCmdLine::SplitLine(int argc, char **argv)

      parse the command line into switches and arguments.

      returns number of switches found
   ------------------------------------------------------*/
   int         SplitLine(int argc, char **argv);

   /*------------------------------------------------------
      bool CCmdLine::HasSwitch(const char *pSwitch)

      was the switch found on the command line ?

      ex. if the command line is : app.exe -a p1 p2 p3 -b p4 -c -d p5

      call                          return
      ----                          ------
      cmdLine.HasSwitch("-a")       true
      cmdLine.HasSwitch("-z")       false
   ------------------------------------------------------*/   
   bool        HasSwitch(const char *pSwitch);

   /*------------------------------------------------------

      StringType CCmdLine::GetSafeArgument(const char *pSwitch, int iIdx, const char *pDefault)

      fetch an argument associated with a switch . if the parameter at
      index iIdx is not found, this will return the default that you
      provide.

      example :
  
      command line is : app.exe -a p1 p2 p3 -b p4 -c -d p5

      call                                      return
      ----                                      ------
      cmdLine.GetSafeArgument("-a", 0, "zz")    p1
      cmdLine.GetSafeArgument("-a", 1, "zz")    p2
      cmdLine.GetSafeArgument("-b", 0, "zz")    p4
      cmdLine.GetSafeArgument("-b", 1, "zz")    zz

   ------------------------------------------------------*/

   StringType  GetSafeArgument(const char *pSwitch, int iIdx, const char *pDefault);

   /*------------------------------------------------------

      StringType CCmdLine::GetArgument(const char *pSwitch, int iIdx)

      fetch a argument associated with a switch. throws an exception 
      of (int)0, if the parameter at index iIdx is not found.

      example :
  
      command line is : app.exe -a p1 p2 p3 -b p4 -c -d p5

      call                             return
      ----                             ------
      cmdLine.GetArgument("-a", 0)     p1
      cmdLine.GetArgument("-b", 1)     throws (int)0, returns an empty string

   ------------------------------------------------------*/
   StringType  GetArgument(const char *pSwitch, int iIdx); 

   /*------------------------------------------------------
      int CCmdLine::GetArgumentCount(const char *pSwitch)

      returns the number of arguments found for a given switch.

      returns -1 if the switch was not found

   ------------------------------------------------------*/
   int         GetArgumentCount(const char *pSwitch);

protected:
   /*------------------------------------------------------

   protected member function
   test a parameter to see if it's a switch :

   switches are of the form : -x
   where 'x' is one or more characters.
   the first character of a switch must be non-numeric!

   ------------------------------------------------------*/
   bool        IsSwitch(const char *pParam);
};

#endif
