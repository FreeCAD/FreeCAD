/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef BASE_FILETEMPLATE_H
#define BASE_FILETEMPLATE_H

#include <string>
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif


namespace Base
{


/** A test class. A more elaborate class description.
 * Detailed description with some formatting:
 *  \par
 *  bla blablablablablablabl:
 *  \code
 *  #include <Base/Console.h>
 *  Base::Console().Log("Stage: %d",i);
 *  \endcode
 *  \par
 *  another blablablablablablablablablabl:
 * Text before the list
 * - list item 1
 *   - sub item 1
 *     - sub sub item 1
 *     - sub sub item 2
 *     .
 *     The dot above ends the sub sub item list.
 *     More text for the first sub item
 *   .
 *   The dot above ends the first sub item.
 *   More text for the first list item
 *   - sub item 2
 *   - sub item 3
 * - list item 2
 * .
 * More text in the same paragraph.
 *
 * More text in a new paragraph.
 * Also with HTML tags:
 *  <ul>
 *  <li> mouse events
 *     <ol>
 *     <li>mouse move event
 *     <li>mouse click event
 *         More info about the click event.
 *     <li>mouse double click event
 *     </ol>
 *  <li> keyboard events
 *     <ol>
 *     <li>key down event
 *     <li>key up event
 *     </ol>
 *  </ul>
 *  More text here.
 *
 * \author YOUR NAME
 */
class BaseExport ClassTemplate
{
public:
    /// Construction
    ClassTemplate();
    /// Destruction
    virtual ~ClassTemplate();

    int testMe(int a,const char *s);

    /**
     * An enum.
     * More detailed enum description.
     */

    enum TEnum {
        TVal1, /**< enum value TVal1. */
        TVal2, /**< enum value TVal2. */
        TVal3  /**< enum value TVal3. */
    }
    *enumPtr{nullptr}, /**< enum pointer. Details. */
    enumVar{TVal1};  /**< enum variable. Details. */

    /**
     * A pure virtual member.
     * @see testMe()
     * @param c1 the first argument.
     * @param c2 the second argument.
     */
    virtual void testMeToo(char c1,char c2) = 0;

    /** @name a group of methods */
    //@{
    /// I am method one
    virtual void one()=0;
    /// I am method two
    virtual void two()=0;
    /// I am method three
    virtual void three()=0;
    //@}


    /**
    * a public variable.
    * Details.
    */
    int publicVar{0};

    /**
     * a function variable.
     * Details.
     */
    int (*handler)(int a,int b){nullptr};

    std::string something;
};

} //namespace Base

#endif // BASE_FILETEMPLATE_H

