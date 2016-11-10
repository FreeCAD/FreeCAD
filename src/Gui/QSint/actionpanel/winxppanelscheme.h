/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef WinXPPanelScheme_H
#define WinXPPanelScheme_H

#include "actionpanelscheme.h"

namespace QSint
{


/**
    \brief WindowsXP-like blue color scheme for ActionPanel and ActionGroup.
    \since 0.2

    \image html ActionPanel2.png Example of the scheme
*/
class QSINT_EXPORT WinXPPanelScheme : public ActionPanelScheme
{
public:
    WinXPPanelScheme();

    static ActionPanelScheme* defaultScheme()
    {
        static WinXPPanelScheme scheme;
        return &scheme;
    }
};


/**
    \brief WindowsXP-like blue color scheme for ActionPanel and ActionGroup (variation 2).
    \since 0.2

    \image html ActionPanel3.png Example of the scheme
*/
class QSINT_EXPORT WinXPPanelScheme2 : public ActionPanelScheme
{
public:
    WinXPPanelScheme2();

    static ActionPanelScheme* defaultScheme()
    {
        static WinXPPanelScheme2 scheme;
        return &scheme;
    }
};


}

#endif // WinXPPanelScheme_H
