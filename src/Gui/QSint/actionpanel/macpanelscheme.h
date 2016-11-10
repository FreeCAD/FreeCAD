/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef MACPANELSCHEME_H
#define MACPANELSCHEME_H

#include "actionpanelscheme.h"


namespace QSint
{


/**
    \brief MacOS-like color scheme for ActionPanel and ActionGroup.
    \since 0.2

    \image html ActionPanel6.png Example of the scheme
*/
class QSINT_EXPORT MacPanelScheme : public ActionPanelScheme
{
public:
    explicit MacPanelScheme();

    static ActionPanelScheme* defaultScheme()
    {
        static MacPanelScheme scheme;
        return &scheme;
    }
};


}

#endif // MACPANELSCHEME_H
