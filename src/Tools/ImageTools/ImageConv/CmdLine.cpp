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

   See SACmds.h for more info.
------------------------------------------------------*/

// if you're using MFC, you'll need to un-comment this line
// #include "stdafx.h"

#include "CmdLine.h"
#ifdef  Q_WS_WIN
#	include "crtdbg.h"
#endif

/*------------------------------------------------------
  int CCmdLine::SplitLine(int argc, char **argv)

  parse the command line into switches and arguments

  returns number of switches found
------------------------------------------------------*/
int CCmdLine::SplitLine(int argc, char **argv)
{
   clear();

   StringType curParam; // current argv[x]

   // skip the exe name (start with i = 1)
   for (int i = 1; i < argc; i++)
   {
      // if it's a switch, start a new CCmdLine
      if (IsSwitch(argv[i]))
      {
         curParam = argv[i];

         StringType arg;

         // look at next input string to see if it's a switch or an argument
         if (i + 1 < argc)
         {
            if (!IsSwitch(argv[i + 1]))
            {
               // it's an argument, not a switch
               arg = argv[i + 1];

               // skip to next
               i++;
            }
            else
            {
               arg = "";
            }
         }

         // add it
         CCmdParam cmd;

         // only add non-empty args
         if (arg != "")
         {
            cmd.m_strings.push_back(arg);
         }

         // add the CCmdParam to 'this'
         pair<CCmdLine::iterator, bool> res = insert(CCmdLine::value_type(curParam, cmd));

      }
      else
      {
         // it's not a new switch, so it must be more stuff for the last switch

         // ...let's add it
 	      CCmdLine::iterator theIterator;

         // get an iterator for the current param
         theIterator = find(curParam);
	      if (theIterator!=end())
         {
            (*theIterator).second.m_strings.push_back(argv[i]);
         }
         else
         {
            // ??
         }
      }
   }

   return size();
}

/*------------------------------------------------------

   protected member function
   test a parameter to see if it's a switch :

   switches are of the form : -x
   where 'x' is one or more characters.
   the first character of a switch must be non-numeric!

------------------------------------------------------*/

bool CCmdLine::IsSwitch(const char *pParam)
{
   if (pParam==NULL)
      return false;

   // switches must non-empty
   // must have at least one character after the '-'
   int len = strlen(pParam);
   if (len <= 1)
   {
      return false;
   }

   // switches always start with '-'
   if (pParam[0]=='-')
   {
      // allow negative numbers as arguments.
      // ie., don't count them as switches
#ifdef  Q_WS_WIN
      return (!isdigit(pParam[1]));
#else
      return true;
#endif
   }
   else
   {
      return false;
   }
}

/*------------------------------------------------------
   bool CCmdLine::HasSwitch(const char *pSwitch)

   was the switch found on the command line ?

   ex. if the command line is : app.exe -a p1 p2 p3 -b p4 -c -d p5

   call                          return
   ----                          ------
   cmdLine.HasSwitch("-a")       true
   cmdLine.HasSwitch("-z")       false
------------------------------------------------------*/

bool CCmdLine::HasSwitch(const char *pSwitch)
{
	CCmdLine::iterator theIterator;
	theIterator = find(pSwitch);
	return (theIterator!=end());
}

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

StringType CCmdLine::GetSafeArgument(const char *pSwitch, int iIdx, const char *pDefault)
{
   StringType sRet;
   
   if (pDefault!=NULL)
      sRet = pDefault;

   try
   {
      sRet = GetArgument(pSwitch, iIdx);
   }
   catch (...)
   {
   }

   return sRet;
}

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

StringType CCmdLine::GetArgument(const char *pSwitch, int iIdx)
{
   if (HasSwitch(pSwitch))
   {
	   CCmdLine::iterator theIterator;

      theIterator = find(pSwitch);
	   if (theIterator!=end())
      {
         if ((*theIterator).second.m_strings.size() > (unsigned)iIdx)
         {
            return (*theIterator).second.m_strings[iIdx];
         }
      }
   }

   throw (int)0;

   return "";
}

/*------------------------------------------------------
   int CCmdLine::GetArgumentCount(const char *pSwitch)

   returns the number of arguments found for a given switch.

   returns -1 if the switch was not found

------------------------------------------------------*/

int CCmdLine::GetArgumentCount(const char *pSwitch)
{
   int iArgumentCount = -1;

   if (HasSwitch(pSwitch))
   {
	   CCmdLine::iterator theIterator;

      theIterator = find(pSwitch);
	   if (theIterator!=end())
      {
         iArgumentCount = (*theIterator).second.m_strings.size();
      }
   }

   return iArgumentCount;
}
